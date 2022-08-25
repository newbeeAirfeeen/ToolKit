/*
* @file_name: sender_queue.hpp
* @date: 2022/08/12
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

#ifndef TOOLKIT_SENDER_BUFFER_HPP
#define TOOLKIT_SENDER_BUFFER_HPP
#include "sliding_window_iterator.hpp"
#include <memory>
#include <utility>
#include <vector>
template<typename _packet_type, typename _duration_type = std::chrono::milliseconds>
class sliding_window {

public:
    using value_type = _packet_type;
    using size_type = size_t;
    using duration_type = _duration_type;
    using pointer = std::shared_ptr<sliding_window<value_type, duration_type>>;
    struct block {
        uint32_t sequence_number = 0;
        uint64_t submit_time_point = 0;
        bool is_retransmit = false;
        value_type content;
    };
    using block_type = std::shared_ptr<block>;
    using iterator = sliding_iterator<block_type>;

public:
    virtual ~sliding_window() = default;
    ///
    virtual void on_packet(const block_type &) = 0;
    /// 主动丢包的范围
    virtual void on_drop_packet(size_type begin, size_type end) = 0;

    void set_max_delay(size_type d) {
        this->_max_delay = d;
    }

    void set_window_size(size_type s) {
        this->_window_size = s;
    }

    void set_initial_sequence(uint32_t s) {
        this->_initial_sequence = s;
    }

    void set_max_sequence(size_type seq) {
        this->_max_sequence = seq;
    }

    size_type get_buffer_size() const {
        return _size.load(std::memory_order_relaxed);
    }

    uint32_t get_initial_sequence() const {
        return _initial_sequence;
    }

    uint32_t get_current_sequence() const {
        return _size.load(std::memory_order_relaxed) == 0 ? 0 : cache[_start]->sequence_number;
    }

    const block_type &get_first_block() const {
        return std::cref(cache[_start]);
    }

    const block_type &get_last_block() const {
        return std::cref(cache[_end]);
    }

    size_type capacity() const {
        return _window_size - get_buffer_size();
    }

    iterator get_flow_part_begin() {
        return iterator(cache, _start, _size.load(std::memory_order_relaxed));
    }

    iterator get_flow_part_end() {
        return iterator(cache, (_end + 1) % cache.size(), 0);
    }

    /// 以发送窗口使用
    void send_in(const block_type &b) {
        //// 拿到当前时间
        b->submit_time_point = get_submit_time();
        //// 序号
        b->sequence_number = _initial_sequence;
        _initial_sequence = (_initial_sequence + 1) % _max_sequence;
        /// 如果小于窗口容量.直接添加尾部
        if (cache.size() < _window_size) {
            cache.push_back(b);
            _end = cache.size() - 1;
            _size.fetch_add(1, std::memory_order_relaxed);
        } else {
            /// 如果碰撞并且容量满
            if (_size.load(std::memory_order_relaxed) == _window_size) {
                /// 删除头部指针。并且移动
                _start = (_start + 1) % _window_size;
                _size.fetch_sub(1, std::memory_order_relaxed);
            }
            _end = (_end + 1) % _window_size;
            cache[_end] = b;
            _size.fetch_add(1, std::memory_order_relaxed);
        }
        /// 尝试直接发送一次
        on_packet(b);
        /// 主动drop
        if (_max_delay)
            drop(b->submit_time_point - _max_delay);
    }

    /// 以接收窗口使用
    void arrived_packet(uint32_t seq, uint32_t receive_time, const value_type &_val) {

        auto _block = std::make_shared<block>();
        _block->sequence_number = seq;
        _block->content = _val;
        _block->submit_time_point = receive_time;

        uint32_t diff = seq - _initial_sequence;
        if (_initial_sequence <= seq && diff >= _window_size) {
            return;
        }

        if (_initial_sequence > seq) {
            diff = _initial_sequence - seq;
            if (diff >= (_max_sequence >> 1)) {
                diff = _max_sequence - diff;
                if (diff >= _window_size) {
                    return;
                }
            }
        }

        auto index = (_start + diff) % _window_size;
        if (cache.size() <= index) {
            cache.resize(index + 1);
            if (index + 1 > _window_size) {
                throw std::invalid_argument("bad index which is greater than window size");
            }
        }
        if (cache[index]) {
            return;
        }
        cache[index] = _block;

        _size.fetch_add(1);

        if (_start <= _end && index >= _end) {
            _end = index;
        } else if (_start <= _end && index < _start) {
            _end = index;
        } else if (_start > _end && _end <= index && _start > index) {
            _end = index;
        }

        /// 输出包
        while (cache[_start] && _size.load(std::memory_order_relaxed) > 0) {
            on_packet(cache[_start]);
            _size.fetch_sub(1);
            cache[_start] = nullptr;
            _initial_sequence = (_initial_sequence + 1) % _max_sequence;
            _start = (_start + 1) % cache.size();
        }
        if (_size.load(std::memory_order_relaxed) == 0 && _start > _end) {
            _start = _end = 0;
        }
        return drop_packet();
    }

    /// 用于发送端
    void sequence_to(size_type seq) {
        if (!get_buffer_size()) {
            return;
        }

        auto begin = get_first_block()->sequence_number;
        while (_size.load(std::memory_order_relaxed) && ((is_cycle() && seq < begin) || (cache[_start]->sequence_number < seq))) {
            cache[_start] = nullptr;
            _start = (_start + 1) % _window_size;
            _size.fetch_sub(1, std::memory_order_relaxed);
        }
        if (_size.load(std::memory_order_relaxed) == 0) {
            _start = _end = 0;
            cache.clear();
        }
    }

    /// 丢弃范围内的包
    void drop(uint32_t begin, uint32_t end) {

        if (get_buffer_size() == 0) {
            return;
        }

        auto first_seq = get_first_block()->sequence_number;
        auto last_seq = get_last_block()->sequence_number;

        /// 回环 且序号落在窗口之外
        if (is_cycle() && end > last_seq) {
            return;
        }

        /// 没有回环 且序号落在窗口之外
        if (!is_cycle() && (end < first_seq || end > last_seq)) {
            return;
        }

        uint32_t diff = 0;
        if (!is_cycle()) {
            diff = end - first_seq + 1;
        } else {
            diff = _max_sequence - first_seq + end + 1;
        }

        while (diff && _size.load(std::memory_order_relaxed) > 0) {
            if (cache[_start]) {
                on_packet(cache[_start]);
                _size.fetch_sub(1);
                cache[_start] = nullptr;
            }
            --diff;
            _start = (_start + 1) % cache.size();
        }
    }

    void try_send_again(size_type begin, size_type end) {
        if (!get_buffer_size()) {
            return on_drop_packet(begin, end);
        }

        auto begin_it = find_block_by_sequence(begin);
        auto end_it = find_block_by_sequence(end);
        auto end_iter = get_flow_part_end();
        if (begin == end || begin_it == end_iter || end_it == end_iter) {
            return on_drop_packet(begin, end);
        }

        Info("try send again {}-{}", begin_it->sequence_number, end_it->sequence_number);
        while (begin_it != end_it) {
            if (*begin_it) {
                Warn("current: {}", begin_it->sequence_number);
                on_packet((*begin_it));
            }
            ++begin_it;
        }
        Warn("current: {}", end_it->sequence_number);
        on_packet((*end_it));
    }

    void clear() {
        _start = _end = 0;
        _size.store(0, std::memory_order_relaxed);
        cache.clear();
    }

    bool is_cycle() const {
        if (!get_buffer_size()) {
            return false;
        }
        auto begin = get_first_block()->sequence_number;
        auto end = get_last_block()->sequence_number;
        return (begin > end) && (begin - end > (_max_sequence >> 1));
    }

    iterator find_block_by_sequence(size_type seq) {
        if (!_size.load(std::memory_order_relaxed)) {
            return iterator(cache, (_end + 1) % cache.size(), 0);
        }
        size_type start = _start;
        size_type tmp_size = _size.load(std::memory_order_relaxed);
        while (tmp_size) {
            if (cache[start]->sequence_number == seq) {
                break;
            }
            start = (start + 1) % cache.size();
            --tmp_size;
        }
        if (tmp_size == 0) {
            return iterator(cache, (_end + 1) % cache.size(), 0);
        }

        return iterator(cache, start, tmp_size);
    }

    size_t get_expected_size() {
        if (_size.load(std::memory_order_relaxed) == 0) {
            return 0;
        }
        uint32_t max = 0, min = 0;
        auto first = _initial_sequence;
        auto last = get_last_block()->sequence_number;
        if (last >= first) {
            max = last;
            min = first;
        } else {
            max = first;
            min = last;
        }

        if ((max - min) >= (_max_sequence >> 1)) {
            return _max_sequence - _initial_sequence + min + 1;
        }

        return max - _initial_sequence + 1;
    }

    /// 拿到窗口中未接收完毕的包
    std::vector<std::pair<uint32_t, uint32_t>> get_pending_seq() {
        if (get_buffer_size() == 0) {
            return {};
        }
        std::vector<std::pair<uint32_t, uint32_t>> vec;

        int64_t begin_seq = -1, end_seq = -1;
        auto begin = _start;
        auto end = (_end + 1) % cache.size();
        auto size = _size.load(std::memory_order_relaxed);
        while (begin != end || size > 0) {
            if (cache[begin]) {
                if (begin_seq >= 0 && end_seq >= 0) {
                    vec.emplace_back(begin_seq, end_seq);
                    begin_seq = end_seq = -1;
                }
                begin = (begin + 1) % cache.size();
                --size;
                continue;
            }

            /// 求出当前的序号
            uint32_t diff = 0;
            if (begin >= _start) {
                diff = begin - _start;
            } else {
                diff = cache.size() - _start + begin;
            }
            uint32_t sequence = (_initial_sequence + diff) % _max_sequence;
            if (begin_seq == -1) {
                begin_seq = end_seq = sequence;
            } else {
                end_seq = sequence;
            }
            begin = (begin + 1) % cache.size();
        }
        if (begin_seq >= 0 && end_seq >= 0) {
            vec.emplace_back(begin_seq, end_seq);
        }
        return vec;
    }

private:
    void drop(uint64_t before_drop_time) {
        if (_size.load(std::memory_order_relaxed) == 0) {
            return;
        }
        iterator iter = this->get_flow_part_begin();
        iterator end = this->get_flow_part_end();

        //// 如果提交的时间点大于丢弃时间，忽略

        if (iter->submit_time_point > before_drop_time) {
            return;
        }

        while (iter != end) {
            /// 如果提交的时间点要大于丢弃的时间点
            if (iter->submit_time_point > before_drop_time) {
                break;
            }
            ++iter;
        }

        on_drop_packet(this->get_flow_part_begin()->sequence_number, iter->sequence_number);
        sequence_to((iter->sequence_number + 1) % _max_sequence);
    }

    inline uint64_t get_submit_time() const {
        auto now = std::chrono::duration_cast<duration_type>(std::chrono::steady_clock::now().time_since_epoch());
        return now.count();
    }

    void drop_packet() {
        /// 主动丢包
        while (time_latency() > _max_delay && _max_delay && _size.load(std::memory_order_relaxed) > 0) {
            auto it = cache[_start];
            if (it) {
                on_drop_packet(it->sequence_number, it->sequence_number);
                cache[_start] = nullptr;
                _size.fetch_sub(1);
            }
            _start = (_start + 1) % cache.size();
            _initial_sequence = (_initial_sequence + 1) % _max_sequence;
        }
    }

    uint32_t time_latency() {
        if (get_buffer_size() == 0 || _max_delay == 0) {
            return 0;
        }
        auto first = get_first_block()->submit_time_point;
        auto last = get_last_block()->submit_time_point;
        uint32_t latency = 0;
        if (last > first) {
            latency = last - first;
        } else {
            latency = first - last;
        }

        if (latency > 0x80000000) {
            latency = 0xFFFFFFFF - latency;
        }
        return latency;
    }

private:
    uint32_t _window_size = 8192;
    uint32_t _initial_sequence = 0;
    uint32_t _max_delay = 0;
    uint32_t _start = 0;
    uint32_t _end = 0;
    std::atomic<uint32_t> _size{0};
    uint32_t _max_sequence = (std::numeric_limits<uint32_t>::max)();
    std::vector<block_type> cache;
};


#endif//TOOLKIT_SENDER_BUFFER_HPP
