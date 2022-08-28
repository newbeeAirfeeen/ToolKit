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
#include "srt_bandwidth.hpp"
#include <chrono>
#include <functional>
#include <list>
template<typename T>
class packet_send_queue : public packet_send_interface<T> {
public:
    using packet_pointer = typename packet_interface<T>::packet_pointer;

private:
    using iterator = typename std::list<packet_pointer>::iterator;

public:
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

    void input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        Trace("current seq={}, time_point={}", seq, time_point);
        if (packet_interface<T>::capacity() <= 0) {
            Trace("capacity is not enough, drop front, seq={}, submit_time={}", _pkt_cache.front()->seq, _pkt_cache.front()->submit_time);
            auto front = std::move(_pkt_cache.front());
            _pkt_cache.pop_front();
            on_drop_packet(front->seq, front->seq);
        }

        auto pkt = std::make_shared<packet<T>>();
        pkt->seq = get_next_sequence();
        pkt->submit_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        pkt->pkt = t;
        on_packet(pkt);
        _pkt_cache.emplace_back(pkt);
        return drop_packet();
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
        _pkt_cache.erase(pair.first, pair.second);
    }

    void clear() override {
        _pkt_cache.clear();
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
            (*pair.first)->is_retransmit = true;
            on_packet(*pair.first);
            ++pair.first;
        }
    }

    void on_packet(const packet_pointer &p) override {
        return packet_interface<T>::_on_packet_func_(p);
    }

    void on_drop_packet(uint32_t begin, uint32_t end) override {
        return packet_interface<T>::_on_drop_packet_func_(begin, end);
    }

private:
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
            _pkt_cache.pop_front();
        }

        if (begin == -1) {
            Debug("there is no packet require drop");
            return;
        }
        Trace("drop packet {}-{}", begin, end);
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

private:
    uint32_t cur_seq = 0;
    std::list<packet_pointer> _pkt_cache;
};


#endif//TOOLKIT_PACKET_SEND_QUEUE_HPP
