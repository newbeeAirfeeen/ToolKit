/*
* @file_name: packet_send_queue.hpp
* @date: 2022/08/26
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

#ifndef TOOLKIT_PACKET_SEND_QUEUE_HPP
#define TOOLKIT_PACKET_SEND_QUEUE_HPP
#include "spdlog/logger.hpp"
#include "srt_ack.hpp"
#include "srt_bandwidth.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <list>
template<typename T>
class packet_send_queue : public packet_send_interface<T>, public std::enable_shared_from_this<packet_send_queue<T>> {
public:
    using packet_pointer = typename packet_interface<T>::packet_pointer;

private:
    using iterator = typename std::list<packet_pointer>::iterator;

public:
    packet_send_queue(asio::io_context &context, const std::shared_ptr<srt::srt_ack_queue> &ack_queue) : context(context), _ack_queue(ack_queue) {
        retransmit_timer = create_deadline_timer<uint32_t, std::chrono::microseconds>(context);
    }

    void set_current_sequence(uint32_t seq) override {
        this->cur_seq = seq;
    }

    uint32_t get_current_sequence() const override {
        return cur_seq;
    }

    uint32_t get_buffer_size() const override {
        return (uint32_t) _pkt_cache.size();
    }

    packet_pointer get_first_block() const override {
        return _pkt_cache.empty() ? nullptr : _pkt_cache.front();
    }

    packet_pointer get_last_block() const override {
        return _pkt_cache.empty() ? nullptr : _pkt_cache.back();
    }

    void start() override {
        std::weak_ptr<packet_send_queue<T>> self(packet_send_queue<T>::shared_from_this());
        retransmit_timer->set_on_expired([self](const uint32_t &v) {
            if (auto stronger_self = self.lock()) {
                return stronger_self->on_timer(v);
            }
        });
    }

    int input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        Trace("current seq={}, time_point={}", seq, time_point);
        if (packet_interface<T>::capacity() <= 0) {
            Trace("capacity is not enough, drop front, seq={}, submit_time={}", _pkt_cache.front()->seq, _pkt_cache.front()->submit_time);
            auto front = std::move(_pkt_cache.front());
            _pkt_cache.pop_front();
            /// 更新总字节数
            _allocated_bytes -= front->pkt->size();
            on_drop_packet(front->seq, front->seq);
        }
        auto pkt = insert_packet(t, this->get_current_sequence());
        /// 尝试直接发送数据
        on_packet(pkt);
        /// 放入lost queue
        drop_packet();
        return static_cast<int>(t->size());
    }

    void drop(uint32_t seq_begin, uint32_t seq_end) override {
        if (_pkt_cache.empty()) {
            Debug("no packet to drop, cache is empty");
            return;
        }

        auto pair = find_packet_by_sequence(seq_begin, seq_end);
        if (pair.first == _pkt_cache.end() || pair.second == _pkt_cache.end()) {
            Debug("no packet to drop, not found begin or end sequence");
            return;
        }
        ++pair.second;

        std::for_each(pair.first, pair.second, [this](const packet_pointer &p) {
            this->_allocated_bytes -= p->pkt->size();
        });
        _pkt_cache.erase(pair.first, pair.second);
        on_size_changed(false, (uint32_t) _pkt_cache.size());
        Trace("after drop, packet cache have size={}", _pkt_cache.size());
    }

    void clear() override {
        retransmit_timer->stop();
        _pkt_cache.clear();
        _allocated_bytes = 0;
    }

    void send_again(uint32_t begin, uint32_t end) override {
        auto pair = find_packet_by_sequence(begin, end);
        if (pair.first == _pkt_cache.end() || pair.second == _pkt_cache.end()) {
            return;
        }
        ++pair.second;
        Debug("send again {}-{}", begin, end);
        while (pair.first != pair.second) {
            /// 设置为重传
            ++((*pair.first)->retransmit_count);
            on_packet(*pair.first);
            ++pair.first;
        }
    }

    void ack_sequence_to(uint32_t seq) override {
        if (_pkt_cache.empty()) {
            return;
        }
        while (!_pkt_cache.empty()) {
            auto element = _pkt_cache.front();
            if (element->seq >= seq && !packet_send_interface<T>::is_cycle()) {
                break;
            }
            _allocated_bytes -= element->pkt->size();
            _pkt_cache.pop_front();
        }
        on_size_changed(false, (uint32_t) _pkt_cache.size());
    }

    void on_packet(const packet_pointer &p) override {
        return packet_interface<T>::_on_packet_func_(p);
    }

    void on_drop_packet(uint32_t begin, uint32_t end) override {
        return packet_interface<T>::_on_drop_packet_func_(begin, end);
    }

    uint64_t get_allocated_bytes() override {
        return this->_allocated_bytes;
    }

    void on_size_changed(bool, uint32_t) override {}
    void update_flow_window(uint32_t) override {}

protected:
    asio::io_context &get_context() {
        return context;
    }

    packet_pointer insert_packet(const T &t, uint32_t seq, uint64_t submit_time = 0) {
        auto pkt = std::make_shared<packet<T>>();
        pkt->seq = seq;
        if (submit_time == 0)
            pkt->submit_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        else
            pkt->submit_time = submit_time;
        pkt->pkt = t;
        /// 增加总字节数目
        _allocated_bytes += pkt->pkt->size();
        _pkt_cache.emplace_back(pkt);
        update_retransmit_timer(pkt->seq);
        return pkt;
    }

    inline uint32_t get_next_sequence() {
        auto seq = cur_seq;
        cur_seq = (cur_seq + 1) % packet_interface<T>::get_max_sequence();
        return seq;
    }

    void drop_packet() {
        int64_t begin = -1, end = -1;
        uint32_t latency = 0;
        while (((latency = packet_interface<T>::get_time_latency()) >= packet_interface<T>::get_max_delay()) && !_pkt_cache.empty()) {
            if (begin == -1)
                begin = _pkt_cache.front()->seq;
            end = _pkt_cache.front()->seq;
            _allocated_bytes -= _pkt_cache.front()->pkt->size();
            _pkt_cache.pop_front();
        }

        if (begin == -1) {
            Debug("there is no packet require drop");
            return;
        }
        Trace("drop packet {}-{}", begin, end);
        on_size_changed(false, (uint32_t) _pkt_cache.size());
        on_drop_packet((uint32_t) begin, (uint32_t) end);
    }

    std::pair<iterator, iterator> find_packet_by_sequence(uint32_t begin, uint32_t end) {

        auto begin_it = _pkt_cache.begin();
        auto end_it = _pkt_cache.end();
        uint32_t index = 0;
        uint32_t cursor_begin = 0, cursor_end = 0;
        typename std::list<packet_pointer>::iterator seq_begin_it = _pkt_cache.end(), seq_end_it = _pkt_cache.end();

        for (; begin_it != end_it; ++begin_it) {
            ++index;
            if ((*begin_it)->seq == begin) {
                seq_begin_it = begin_it;
                cursor_begin = index;
            }
            if ((*begin_it)->seq == end) {
                seq_end_it = begin_it;
                cursor_end = index;
                break;
            }
        }

        if (seq_begin_it == _pkt_cache.end() || seq_end_it == _pkt_cache.end()) {
            return {_pkt_cache.end(), _pkt_cache.end()};
        }

        if (cursor_begin > cursor_end) {
            std::swap(seq_begin_it, seq_end_it);
        }

        return {seq_begin_it, seq_end_it};
    }

    void update_retransmit_timer(uint32_t seq, uint32_t counts = 1) {
        auto rto = _ack_queue->get_rto();
        auto rtt_var = _ack_queue->get_rtt_var();
        auto RTO = counts * (rto + 4 * rtt_var + 20000) + 10000;
        /// 当前这个包的重传时间
        Trace("set retransmit packet time, RTO={}", RTO);
        retransmit_timer->add_expired_from_now(RTO, seq);
    }

    /// 超时重传
    void on_timer(const uint32_t &v) {
        if (_pkt_cache.empty()) {
            ///Trace("pkt cache empty, stopping timer");
            return;
        }

        auto diff = packet_interface<T>::sequence_diff(get_current_sequence(), v);
        if (diff > packet_interface<T>::get_window_size()) {
            Trace("out of window or too old packet to send");
            return;
        }


        auto iter = find_packet_by_sequence(v, v);
        if (iter.first == _pkt_cache.end()) {
            Warn("the packet is missing, seq={}", v);
            return;
        }
        const auto &pkt_pointer = (*iter.first);
        /// 超时丢弃
        auto now = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - pkt_pointer->submit_time;
        if (latency >= packet_interface<T>::get_max_delay()) {
            Trace("pkt retransmit time out, drop it, latency={}", latency);
            on_drop_packet(pkt_pointer->seq, pkt_pointer->seq);
            _allocated_bytes -= (*iter.first)->pkt->size();
            _pkt_cache.erase(iter.first);
            on_size_changed(false, (uint32_t) _pkt_cache.size());
            return;
        }

        //// 重传数据包
        ++pkt_pointer->retransmit_count;
        on_packet(pkt_pointer);
        /// 更新下一次重传的时间
        update_retransmit_timer(pkt_pointer->seq, pkt_pointer->retransmit_count);
    }

protected:
    uint32_t cur_seq = 0;
    std::list<packet_pointer> _pkt_cache;
    uint64_t _allocated_bytes = 0;
    std::shared_ptr<deadline_timer<uint32_t, std::chrono::microseconds>> retransmit_timer;
    std::shared_ptr<srt::srt_ack_queue> _ack_queue;
    asio::io_context &context;
};


#endif//TOOLKIT_PACKET_SEND_QUEUE_HPP
