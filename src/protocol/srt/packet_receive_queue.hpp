/*
* @file_name: packet_receive_queue.hpp
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

#ifndef TOOLKIT_PACKET_RECEIVE_QUEUE_HPP
#define TOOLKIT_PACKET_RECEIVE_QUEUE_HPP
#include <vector>
template<typename T>
class packet_receive_queue : public packet_receive_interface<T> {
public:
    using packet_pointer = typename packet_interface<T>::packet_pointer;

private:
    using iterator = typename std::map<uint32_t, packet_pointer>::iterator;

public:
    ~packet_receive_queue() override = default;
    void set_current_sequence(uint32_t seq) override {
        this->_cur_seq = seq;
    }

    uint32_t get_current_sequence() const override {
        return this->_cur_seq;
    }

    uint32_t get_buffer_size() const override {
        return _size;
    }

    packet_pointer get_first_block() const override {
        if (_size <= 0) {
            return nullptr;
        }

        uint32_t i = _start;
        while (1) {
            if (_pkt_buf[i]) {
                return _pkt_buf[i];
            }
            i = (i + 1) % _pkt_buf.size();
        }
    }

    packet_pointer get_last_block() const override {
        if (_size <= 0) {
            return nullptr;
        }
        uint32_t i = (_end + (uint32_t) _pkt_buf.size() - 1) % (uint32_t) _pkt_buf.size();
        return _pkt_buf[i];
    }

    void set_window_size(uint32_t size) override {
        packet_interface<T>::set_window_size(size);
        _pkt_buf.resize(size);
    }

    int input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        Trace("input packet, seq={}, time_point={}", seq, time_point);
        /// 窗口已经满了
        while (_size > 0 && _start == _end) {
            if (_pkt_buf[_start]) {
                Debug("capacity is full, pop the front, seq={}, size={}", _pkt_buf[_start]->seq, _size);
                on_packet(_pkt_buf[_start]);
                --_size;
                _pkt_buf[_start] = nullptr;
            }
            _start = (_start + 1) % _pkt_buf.size();
            _cur_seq = (_cur_seq + 1) % packet_interface<T>::get_max_sequence();
        }

        uint32_t diff = 0;
        if (_cur_seq <= seq) {
            diff = seq - _cur_seq;
            if (diff >= _pkt_buf.size()) {
                Debug("too new packet seq, current seq={}, seq={}, diff={}", _cur_seq, seq, diff);
                return -1;
            }
        } else {
            diff = _cur_seq - seq;
            if (diff >= (packet_interface<T>::get_max_sequence() >> 1)) {
                diff = packet_interface<T>::get_max_sequence() - diff;
                if (diff >= _pkt_buf.size()) {
                    Debug("cycle packet too new packet seq, current seq={}, seq={}, diff={}", _cur_seq, seq, diff);
                    return -1;
                }
            }
        }

        auto pos = (_start + diff) % _pkt_buf.size();
        if (_pkt_buf[pos]) {
            Debug("same seq packet, ignore it, seq={}", _pkt_buf[pos]->seq);
            return -1;
        }

        auto pkt = std::make_shared<packet<T>>();
        pkt->seq = seq;
        pkt->pkt = t;
        pkt->submit_time = time_point;
        _pkt_buf[pos] = pkt;
        ++_size;

        if (_start <= _end && pos >= _end) {
            _end = (pos + 1) % (uint32_t) _pkt_buf.size();
        }

        if (_start <= _end && pos < _start) {
            _end = (pos + 1) % (uint32_t) _pkt_buf.size();
        }

        if (_start > _end && _end <= pos && _start > pos) {
            _end = (pos + 1) % (uint32_t) _pkt_buf.size();
        }

        Trace("after input packet, start={}, end={}, size={}", _start, _end, _size);

        auto it = _pkt_buf[_start];
        while (it) {
            on_packet(it);
            --_size;
            _pkt_buf[_start] = nullptr;
            _cur_seq = (_cur_seq + 1) % packet_interface<T>::get_max_sequence();
            _start = (_start + 1) % _pkt_buf.size();
            it = _pkt_buf[_start];
        }

        auto time_latency = packet_interface<T>::get_max_delay();
        while (packet_interface<T>::get_time_latency() > time_latency && time_latency) {
            auto iter = _pkt_buf[_start];
            if (iter) {
                Debug("drop the time packet, seq={}, time_point={}, size={}, _start={}, _end={}", _pkt_buf[_start]->seq, _pkt_buf[_start]->submit_time, _size, _start, _end);
                _pkt_buf[_start] = nullptr;
                on_packet(iter);
                --_size;
            }
            _cur_seq = (_cur_seq + 1) % packet_interface<T>::get_max_sequence();
            _start = (_start + 1) % _pkt_buf.size();
        }
        return 0;
    }

    void drop(uint32_t seq_begin, uint32_t seq_end) override {
        uint32_t diff = 0;
        /// 回环后的步长
        if (is_seq_cycle(_cur_seq, seq_end) && seq_end < _cur_seq) {
            diff = packet_interface<T>::get_max_sequence() - _cur_seq + seq_end + 1;
        } else {
            if (seq_end < _cur_seq) {
                Warn("step _cur_seq size={}, seq_end={}, ignore it", _cur_seq, seq_end);
                return;
            }
            diff = seq_end - _cur_seq + 1;
        }

        auto expected_size = get_expected_size();
        if (diff > expected_size) {
            Warn("step size={}, expected_size={}, ignore it", diff, expected_size);
            return;
        }

        uint32_t seq_ = _cur_seq;
        for (uint32_t i = 0; i < diff; i++) {
            auto pos = (i + _start) % _pkt_buf.size();
            if (_pkt_buf[pos]) {
                on_packet(_pkt_buf[pos]);
                _pkt_buf[pos] = nullptr;
                --_size;
            } else {
                on_drop_packet(seq_, seq_);
            }
            seq_ = (seq_ + 1) % packet_interface<T>::get_max_sequence();
        }

        _cur_seq = (seq_end + 1) % packet_interface<T>::get_max_sequence();
        _start = (diff + _start) % _pkt_buf.size();
        if (_size <= 0) {
            _end = _start;
        }
    }

    void clear() override {
        _pkt_buf.clear();
        _start = _end = _size = 0;
        _pkt_buf.resize(packet_interface<T>::get_window_size());
    }

    void on_packet(const packet_pointer &p) override {
        return packet_interface<T>::_on_packet_func_(p);
    }

    void on_drop_packet(uint32_t begin, uint32_t end) override {
        return packet_interface<T>::_on_drop_packet_func_(begin, end);
    }

    uint32_t get_expected_size() const override {
        auto first = get_first_block();
        auto last = get_last_block();

        if (!first || !last) {
            Warn("first or last is empty!!");
            return 0;
        }
        uint32_t max = 0, min = 0;
        if (last->seq >= first->seq) {
            max = last->seq;
            min = first->seq;
        } else {
            max = first->seq;
            min = last->seq;
        }

        if ((max - min) >= (packet_interface<T>::get_max_sequence() >> 1)) {
            return packet_interface<T>::get_max_sequence() - get_current_sequence() + min + 1;
        }
        return max - get_current_sequence() + 1;
    }

    std::vector<std::pair<uint32_t, uint32_t>> get_pending_packets() const override {
        if (_size <= 0) {
            Trace("no pending packets.");
            return {};
        }
        std::vector<std::pair<uint32_t, uint32_t>> vec;
        int64_t begin_seq = -1, end_seq = -1;
        auto begin = _start;
        auto end = _end;
        auto size = _size;
        Debug("pending packet, begin={}, end={}, size={}", begin, end, size);
        while (begin != end && size > 0) {
            if (_pkt_buf[begin]) {
                if (begin_seq >= 0 && end_seq >= 0) {
                    Debug("insert nak pair, {}-{}", begin_seq, end_seq);
                    vec.emplace_back((uint32_t) begin_seq, (uint32_t) end_seq);
                    begin_seq = end_seq = -1;
                }
                begin = (begin + 1) % _pkt_buf.size();
                --size;
                continue;
            }

            /// 求出当前的序号
            uint32_t diff = 0;
            if (begin >= _start) {
                diff = begin - _start;
            } else {
                diff = (uint32_t) _pkt_buf.size() - _start + begin;
            }
            uint32_t sequence = (_cur_seq + diff) % packet_interface<T>::get_max_sequence();
            if (begin_seq == -1) {
                begin_seq = end_seq = sequence;
            } else {
                end_seq = sequence;
            }
            begin = (begin + 1) % _pkt_buf.size();
        }
        if (begin_seq >= 0 && end_seq >= 0) {
            Debug("insert nak pair, {}-{}", begin_seq, end_seq);
            vec.emplace_back((uint32_t) begin_seq, (uint32_t) end_seq);
        }
        return vec;
    }


private:
    bool is_seq_cycle(uint32_t first, uint32_t second) {
        uint32_t diff = 0;
        if (first > second) {
            diff = first - second;
        } else {
            diff = second - first;
        }
        if (diff > (packet_interface<T>::get_max_sequence() >> 1)) {
            return true;
        }
        return false;
    }

private:
    std::vector<packet_pointer> _pkt_buf;
    uint32_t _start = 0;
    uint32_t _end = 0;
    uint32_t _size = 0;
    uint32_t _cur_seq = 0;
};


#endif//TOOLKIT_PACKET_RECEIVE_QUEUE_HPP
