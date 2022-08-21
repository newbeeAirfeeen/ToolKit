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
#include "deadline_timer.hpp"
#include <memory>
#include <vector>
template<typename type>
class sliding_iterator : public std::iterator<std::forward_iterator_tag, type> {
public:
    using size_type = size_t;
    sliding_iterator(std::vector<type> &target, size_type index, const size_type &_size) : _target(&target), _index(index), _size(_size) {}
    sliding_iterator &operator++() {
        _index = (_index + 1) % (*_target).size();
        --_size;
        return *this;
    }

    sliding_iterator operator++(int) {
        auto tmp = _index;
        auto size = _size;
        _index = (_index + 1) % (*_target).size();
        --_size;
        return sliding_iterator<type>(*_target, tmp, size);
    }

    type &operator*() {
        return std::ref(_target->operator[](_index));
    }

    type &operator->() {
        return std::ref(_target->operator[](_index));
    }

    bool operator!=(const sliding_iterator<type> &other) {
        if ((*_target) != (*other._target)) {
            return true;
        }

        if (_index != other._index) {
            return true;
        }

        if (_size != other._size) {
            return true;
        }
        return false;
    }


    bool operator==(const sliding_iterator<type> &other) const {
        return _index == other._index && (*_target) == (*other._target) && _size == other._size;
    }

private:
    std::vector<type> *_target = nullptr;
    size_type _index = 0;
    size_type _size;
};

template<typename T>
bool operator==(const sliding_iterator<T> &lhs, const sliding_iterator<T> &rhs) {
    return lhs.operator==(rhs);
}

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
    virtual pointer get_shared_from_this() = 0;
    ///
    virtual void on_packet(const block_type &) = 0;
    /// 主动丢包的范围
    virtual void on_drop_packet(size_type begin, size_type end) = 0;

public:
    explicit sliding_window(asio::io_context &context) : context(context) {
        timer = create_deadline_timer<int, duration_type>(context);
    }

    void start() {
        std::weak_ptr<sliding_window<value_type, duration_type>> self(get_shared_from_this());
        timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            stronger_self->on_timer(v);
        });
    }

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

    iterator begin() {
        return iterator(cache, _start, _size.load(std::memory_order_relaxed));
    }

    iterator end() {
        return iterator(cache, (_end + 1) % cache.size(), 0);
    }

    void insert(const block_type &b) {
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
        /// 更新定时器
        if (_max_delay) {}
        //timer->add_expired_from_now(_max_delay, 1);
    }

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
    }


    void try_send_again(size_type begin, size_type end) {
        if (!get_buffer_size()) {
            return on_drop_packet(begin, end);
        }

        auto begin_it = find_block_by_sequence(begin);
        if (begin == end) {
            if (begin_it != this->end()) {
                return on_packet(*begin_it);
            }
            return on_drop_packet(begin, end);
        }

        auto end_it = find_block_by_sequence(end);
        if (end_it == this->end()) {
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

    void stop() {
        cache.clear();
        timer->stop();
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
            return iterator(cache, (_end + 1) % _window_size, 0);
        }
        size_type start = _start;
        size_type tmp_size = _size.load(std::memory_order_relaxed);
        while (tmp_size) {
            if (cache[start]->sequence_number == seq) {
                break;
            }
            start = (start + 1) % _window_size;
            --tmp_size;
        }
        if (tmp_size == 0) {
            return iterator(cache, (_end + 1) % _window_size, 0);
        }

        return iterator(cache, start, tmp_size);
    }

private:
    void on_timer(const int &) {
        if (!_size.load(std::memory_order_relaxed)) {
            return;
        }
        auto deadline_time = get_submit_time() - _max_delay;
        size_type begin = 0;
        size_type end = 0;
        bool is_drop = false;
        while (_size.load(std::memory_order_relaxed) && (get_first_block()->submit_time_point <= deadline_time)) {
            if (!begin)
                begin = get_first_block()->sequence_number;
            end = get_first_block()->sequence_number;
            cache[_start] = nullptr;
            _start = (_start + 1) % _window_size;
            _size.fetch_sub(1, std::memory_order_relaxed);
            is_drop = true;
        }
        if (!is_drop) {
            return;
        }
    }

    inline uint64_t get_submit_time() const {
        auto now = std::chrono::duration_cast<duration_type>(std::chrono::steady_clock::now().time_since_epoch());
        return now.count();
    }

private:
    asio::io_context &context;
    std::shared_ptr<deadline_timer<int, _duration_type>> timer;
    size_type _window_size = 8192;
    uint32_t _initial_sequence = 0;
    size_type _max_delay = 0;
    size_type _start = 0;
    size_type _end = 0;
    std::atomic<size_type> _size{0};
    size_type _max_sequence = (std::numeric_limits<size_type>::max)();
    std::vector<block_type> cache;
};


#endif//TOOLKIT_SENDER_BUFFER_HPP
