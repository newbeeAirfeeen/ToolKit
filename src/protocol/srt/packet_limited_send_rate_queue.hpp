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
#include "packet_sending_queue.hpp"
#include "spdlog/logger.hpp"
#include "srt_congestion.hpp"
#include "srt_packet.h"
#include <atomic>
#include <map>
#include <mutex>
template<typename T>
class packet_limited_send_rate_queue : public packet_sending_queue<T>, public congestion_holder {
public:
    using base_type = packet_sending_queue<T>;
    using packet_pointer = typename base_type::packet_pointer;

private:
    using duration_type = std::chrono::nanoseconds;

public:
    packet_limited_send_rate_queue(const event_poller::Ptr &poller,
                                   const std::shared_ptr<srt::srt_ack_queue> &ack,
                                   bool enable_retransmit,
                                   uint32_t sock_id,
                                   uint32_t max_payload,
                                   const std::chrono::steady_clock::time_point &t,
                                   uint16_t payload = 1456) : base_type(poller, ack, enable_retransmit), _size(0), timer(poller->get_executor()) {
        avg_payload_size = payload > 1456 ? 1472 : (payload + 16);
        Trace("average payload size={}", avg_payload_size);
        this->_sock_id = sock_id;
        this->_conn = t;
        this->max_payload = max_payload;
        update_snd_period();
        _congestion = std::make_shared<congestion>(*this);
    }
    ~packet_limited_send_rate_queue() override = default;

public:
    void start() override {
        Trace("the temporary buffer cache initial size={}", this->get_window_size());
        _size.store(this->get_window_size(), std::memory_order_relaxed);
    }

    int input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        /// 更新输入率
        /// Trace("input packet to bandwidth, size={}", t->size());
        packet_send_interface<T>::get_bandwidth_mode()->input_packet((uint16_t) t->size());

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
                return static_cast<int>(t->size());
            }
            _is_commit = true;
        }

        /// 如果比较成功，说明在进程中..
        std::weak_ptr<packet_limited_send_rate_queue<T>> self(std::static_pointer_cast<packet_limited_send_rate_queue<T>>(base_type::shared_from_this()));
        base_type::get_poller()->async([self]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            /// 直接调用
            stronger_self->on_timer();
        });
        return static_cast<int>(t->size());
    }

    ///(1) On sending a data packet (either original or retransmitted),
    /// update the value of average packet payload size (AvgPayloadSize):
    void on_packet(const packet_pointer &p) override {
        update_avg_payload(static_cast<uint16_t>(p->pkt->size()));
        base_type::on_packet(p);
    }

    void ack_sequence_to(bool full_ack, uint32_t seq, uint32_t receive_rate, uint32_t link_capacity) override {
        /// 先更新拥塞控制
        if (full_ack) {
            _congestion->ack_sequence_to(seq, receive_rate, link_capacity);
            this->_receive_rate = receive_rate;
        }
        base_type::ack_sequence_to(full_ack, seq, receive_rate, link_capacity);
        /// 更新参数值
        this->_last_ack_number.store(seq);
        // auto average_size = this->get_allocated_bytes() / this->get_buffer_size();
        // update_avg_payload(average_size == 0 ? 1456 : average_size);
        update_snd_period();
    }

    void send_again(uint32_t begin, uint32_t end) override {
        /// 更新拥塞控制
        _congestion->rexmit_pkt_event(true, begin, end);
        packet_sending_queue<T>::send_again(begin, end);
    }

    void on_size_changed(bool full, uint32_t size) override {
        if (full || size >= this->get_window_size()) {
            ///Trace("window is full, wait not full to recover");
            return;
        }
        //// 重新开启定时器
        bool _not_empty = false;
        {
            std::lock_guard<std::recursive_mutex> lmtx(mtx);
            _is_commit = size != 0;
            if (!_buffer_cache.empty() && !_is_commit) {
                _not_empty = _is_commit = true;
            }
        }
        /// 重新开启定时器
        if (_not_empty) {
            on_timer();
        }
    }

    void update_flow_window(uint32_t cwnd) override {
        flow_window = cwnd;
        Trace("update flow window, peer cwnd={}, current window size={}", flow_window, this->get_window_size());
    }

    void clear() override {
        base_type::clear();
        timer.cancel();
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        _buffer_cache.clear();
        _is_commit = false;
        _size.store(this->get_window_size());
    }

    void rexmit_packet(const packet_pointer &p) override {
        /// ON RTO
        _congestion->rexmit_pkt_event(false, 0, 0);
        update_avg_payload(static_cast<uint16_t>(p->pkt->size()));
        base_type::rexmit_packet(p);
    }

public:
    //// congestion holder
    uint32_t get_current_seq() const override {
        return packet_sending_queue<T>::get_current_sequence();
    }

    uint32_t get_RTT() const override {
        return packet_sending_queue<T>::get_ack_queue()->get_rto();
    }

    uint32_t get_ack_last_number() const override {
        return _last_ack_number.load(std::memory_order_relaxed);
    }

    uint32_t get_lost_list_size() const override {
        return this->get_buffer_size();
    }

    uint32_t get_max_window_size() const override {
        return packet_sending_queue<T>::get_window_size();
    }

    uint32_t get_max_payload() const override {
        return max_payload;
    }

    uint32_t get_deliver_rate() const override {
        return _receive_rate;
    }

private:
    void on_timer() {
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
        auto seq = this->get_next_sequence();
        srt::srt_packet pkt;
        pkt.set_control(false);
        pkt.set_packet_sequence_number(seq);
        pkt.set_message_number(this->get_next_packet_message_number());
        pkt.set_timestamp(static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(now - _conn).count()));
        pkt.set_socket_id(_sock_id);
        auto pkt_buf = create_packet(pkt);

        pkt_buf->append(t->data(), t->size());
        auto p = base_type::insert_packet(pkt_buf, seq, std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        /// 尝试发送数据
        on_packet(p);
        {
            std::lock_guard<std::recursive_mutex> lmtx(mtx);
            if (_buffer_cache.empty()) {
                _is_commit = false;
                return;
            }
        }
        ////////
        uint64_t _next_send_point = 1000;
        /// 如果在慢启动阶段
        if (_congestion->slow_starting()) {
            _next_send_point = (uint64_t) (_congestion->get_send_period() * 1000);
        } else {
            /// 计算当前时间(纳秒)
            auto now_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            if (!_last_send_point) {
                _last_send_point = now_nano;
            }
            /// 计算出当前时间点离上次发送点的间隔
            auto internal = now_nano - _last_send_point;
            /// 并更新为当前的发送时间点
            _last_send_point = now_nano;

            /// 计算出下一次应该发送的时间间隔
            _next_send_point = (uint64_t) (_pkt_snd_period * 1000);
            /// 拿到拥塞控制发送的建议时间间隔
            auto _congestion_period = (uint64_t) (_congestion->get_send_period() * 1000);
            Trace("congestion_period={} ns, next_send_point={} ns", _congestion_period, _next_send_point);
            if (_next_send_point < _congestion_period) {
                _next_send_point = _congestion_period;
            }

            /// 如果间隔大于下一次发送时间
            if (internal > _next_send_point) {
                return on_timer();
            }
        }

        Trace("next send duration={} ns", _next_send_point);
        std::weak_ptr<packet_limited_send_rate_queue<T>> self(std::static_pointer_cast<packet_limited_send_rate_queue<T>>(base_type::shared_from_this()));
        timer.expires_after(duration_type(_next_send_point));
        timer.async_wait([self](const std::error_code &e) {
            auto stronger_self = self.lock();
            if (!stronger_self || e) {
                return;
            }
            stronger_self->on_timer();
        });
    }

private:
    bool wait_capacity() {
        /// 如果窗口的容量 和 发送
        uint32_t cwnd = std::min(flow_window, this->get_window_size());
        cwnd = std::min(cwnd, _congestion->get_cwnd_window());
        auto diff = packet_interface<T>::sequence_diff(_last_ack_number.load(), this->get_current_sequence());
        if (this->capacity() <= 0 || diff >= cwnd) {
            Debug("wait capacity, cwnd={}, diff_seq={}, flow_window={}, capacity={}", cwnd, diff, flow_window, this->capacity());
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
        _pkt_snd_period = avg_payload_size * 1e6 / base_type::get_bandwidth_mode()->get_bandwidth() * 1.0;
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
    asio::steady_timer timer;
    std::recursive_mutex mtx;
    std::list<T> _buffer_cache;
    std::atomic<int> _size;
    bool _is_commit = false;
    uint32_t _sock_id = 0;
    uint32_t max_payload = 1500;
    std::chrono::steady_clock::time_point _conn;
    /// 上一次发送包的序号
    uint32_t message_number = 1;
    uint32_t flow_window = 0;
    uint32_t _receive_rate = 0;
    std::atomic<uint32_t> _last_ack_number{0};
    uint64_t _last_send_point = 0;
    std::shared_ptr<congestion> _congestion;
};


#endif//TOOLKIT_PACKET_LIMITED_SEND_RATE_QUEUE_HPP
