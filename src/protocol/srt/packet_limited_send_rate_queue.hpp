/*
* @file_name: packet_limited_send_queue.hpp
* @date: 2022/08/27
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef TOOLKIT_PACKET_LIMITED_SEND_RATE_QUEUE_HPP
#define TOOLKIT_PACKET_LIMITED_SEND_RATE_QUEUE_HPP
#include "deadline_timer.hpp"
#include "packet_send_queue.hpp"
#include "spdlog/logger.hpp"
#include "srt_packet.h"
#include <atomic>
#include <map>
#include <mutex>
template<typename T>
class packet_limited_send_rate_queue : public packet_send_queue<T> {
public:
    using packet_pointer = typename packet_send_queue<T>::packet_pointer;

private:
    using duration_type = std::chrono::nanoseconds;

public:
    packet_limited_send_rate_queue(asio::io_context &io_context,
                                   const std::shared_ptr<srt::srt_ack_queue> &ack,
                                   uint32_t sock_id,
                                   const std::chrono::steady_clock::time_point &t,
                                   uint16_t payload = 1456) : packet_send_queue<T>(io_context, ack), _size(0) {
        Trace("create limited send queue, payload={}", payload);
        timer = create_deadline_timer<int, duration_type>(io_context);
        avg_payload_size = payload > 1456 ? 1472 : (payload + 16);
        Trace("average payload size={}", avg_payload_size);
        this->_sock_id = sock_id;
        this->_conn = t;
        update_snd_period();
    }
    ~packet_limited_send_rate_queue() override = default;

public:
    void start() override {
        packet_send_queue<T>::start();
        std::weak_ptr<packet_limited_send_rate_queue<T>> self(std::static_pointer_cast<packet_limited_send_rate_queue<T>>(packet_send_queue<T>::shared_from_this()));
        timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (!stronger_self) { return; }
            return stronger_self->on_timer(v);
        });
        Trace("the temporary buffer cache initial size={}", this->get_window_size());
        _size.store(this->get_window_size(), std::memory_order_relaxed);
    }

    int input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        /// 更新输入率
        /// Trace("input packet to bandwidth, size={}", t->size());
        packet_send_interface<T>::get_bandwidth_mode()->input_packet(t->size());

        auto size = _size.load(std::memory_order_relaxed);
        if (wait_capacity() || size <= 0) {
            //Trace("window temporary size={}, wait it...", size);
            return 0;
        }
        {
            std::lock_guard<std::recursive_mutex> lmtx(mtx);
            _buffer_cache.push_back(t);
            _size.fetch_sub(1);
            if (_is_commit) {
                Trace("already in sending proc..");
                return static_cast<int>(t->size());
            }
            _is_commit = true;
        }

        /// 如果比较成功，说明在进程中..
        std::weak_ptr<packet_limited_send_rate_queue<T>> self(std::static_pointer_cast<packet_limited_send_rate_queue<T>>(packet_send_queue<T>::shared_from_this()));
        packet_send_queue<T>::get_context().post([self]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            stronger_self->timer->add_expired_from_now(0, 1);
        });
        return static_cast<int>(t->size());
    }

    ///(1) On sending a data packet (either original or retransmitted),
    /// update the value of average packet payload size (AvgPayloadSize):
    void on_packet(const packet_pointer &p) override {
        update_avg_payload(static_cast<uint16_t>(p->pkt->size()));
    }

    void ack_sequence_to(uint32_t seq) override {
        packet_send_queue<T>::ack_sequence_to(seq);
        _last_ack_number = seq;
        // auto average_size = this->get_allocated_bytes() / this->get_buffer_size();
        // update_avg_payload(average_size == 0 ? 1456 : average_size);
        update_snd_period();
    }


    void on_size_changed(bool full, uint32_t size) override {
        if (full || size >= this->get_window_size()) {
            ///Trace("window is full, wait not full to recover");
            return;
        }
        //// 重新开启定时器
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        if (!_buffer_cache.empty() && !_is_commit) {
            timer->add_expired_from_now(0, 1);
            _is_commit = true;
        }
    }

    void update_flow_window(uint32_t cwnd) override {
        flow_window = cwnd;
        Trace("update flow window, peer cwnd={}, current window size={}", flow_window, this->get_window_size());
    }

private:
    void on_timer(const int &v) {
        if (wait_capacity()) {
            return;
        }

        T t;
        {
            std::lock_guard<std::recursive_mutex> lmtx(mtx);
            if (_buffer_cache.empty()) {
                _is_commit = false;
                return;
            }
            t = std::move(_buffer_cache.front());
            _buffer_cache.pop_front();
            _size.fetch_add(1);
        }

        auto now = std::chrono::steady_clock::now();

        srt::srt_packet pkt;
        pkt.set_control(false);
        pkt.set_packet_sequence_number(this->get_next_sequence());
        pkt.set_message_number(this->get_next_packet_message_number());
        pkt.set_timestamp(static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(now - _conn).count()));
        pkt.set_socket_id(_sock_id);
        auto pkt_buf = create_packet(pkt);

        pkt_buf->append(t->data(), t->size());
        update_avg_payload(t->size());
        auto now_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        if (!_last_send_point) {
            _last_send_point = now_nano;
        }

        auto p = packet_send_queue<T>::insert_packet(pkt_buf, this->get_current_sequence(), std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        /// 尝试发送数据
        packet_send_queue<T>::on_packet(p);
        this->drop_packet();

        auto internal = (now_nano - _last_send_point) / 1e3;
        auto _next_send_point = ((uint64_t) _pkt_snd_period * 1000);
        _last_send_point = now_nano;
        //// 更新发送间隔
        {
            std::lock_guard<std::recursive_mutex> lmtx(mtx);
            if (_buffer_cache.empty()) {
                _is_commit = false;
                return;
            }
        }
        timer->add_expired_from_now(internal > _next_send_point ? 0 : _next_send_point, 1);
    }

private:
    inline bool wait_capacity() {
        /// 如果窗口的容量 和 发送
        auto cwnd = std::min(flow_window, this->get_window_size());
        auto diff = packet_interface<T>::sequence_diff(_last_ack_number, this->get_current_sequence());
        if (this->capacity() <= 0 || diff >= cwnd) {
            Warn("window is full, cwnd={}, diff_seq={}, flow_window={}, capacity={}", cwnd, diff, flow_window, this->capacity());
            return true;
        }
        return false;
    }

    inline uint32_t get_next_packet_message_number() {
        auto tmp = message_number;
        message_number = (message_number + 1) % 0x3FFFFFF;
        return tmp;
    }

    inline void update_avg_payload(uint16_t size) {
        avg_payload_size = static_cast<uint16_t>(7.0 / 8 * avg_payload_size + 1.0 / 8 * size);
        Trace("update average payload size={}", avg_payload_size);
    }

    inline void update_snd_period() {
        /// 求出步长
        _pkt_snd_period = avg_payload_size * 1e6 / packet_send_queue<T>::get_bandwidth_mode()->get_bandwidth() * 1.0;
        Trace("update packet send period={} us", static_cast<uint64_t>(_pkt_snd_period));
    }

private:
    /// AvgPayloadSize is equal to the maximum
    /// allowed packet payload size, which cannot be larger than 1456 bytes.
    /// AvgPayloadSize = 7/8 * AvgPayloadSize + 1/8 * PacketPayloadSize
    uint16_t avg_payload_size = 1472;
    /// PKT_SND_PERIOD = PktSize * 1000000 / MAX_BW
    ///  microseconds
    double _pkt_snd_period = 11.776;
    std::shared_ptr<deadline_timer<int, duration_type>> timer;
    std::recursive_mutex mtx;
    std::list<T> _buffer_cache;
    std::atomic<int> _size;
    bool _is_commit = false;
    uint32_t _sock_id = 0;
    std::chrono::steady_clock::time_point _conn;
    /// 上一次发送包的序号
    uint32_t message_number = 1;
    uint32_t flow_window = 0;
    uint32_t _last_ack_number = 0;
    uint64_t _last_send_point = 0;
};


#endif//TOOLKIT_PACKET_LIMITED_SEND_RATE_QUEUE_HPP
