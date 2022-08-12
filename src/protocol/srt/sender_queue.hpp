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
#include <list>
#include <memory>
template<typename PACKET_TYPE, typename _duration_type = std::chrono::milliseconds, typename allocator = std::allocator<PACKET_TYPE>>
class sender_queue {

public:
    using value_type = PACKET_TYPE;
    using size_type = size_t;
    using duration_type = _duration_type;
    using pointer = std::shared_ptr<sender_queue<value_type, duration_type, allocator>>;
    template<typename R>
    struct rebind_alloc {
        using type = std::allocator<R>;
    };

    struct block {
        size_type sequence_number = 0;
        uint64_t submit_time_point = 0;
        value_type content;
    };
    using block_type = std::shared_ptr<block>;

public:
    virtual ~sender_queue() = default;
    virtual pointer get_shared_from_this() = 0;
    virtual void send(const block_type &) = 0;
    /// 主动丢包的个数
    virtual void on_drop_packet(size_type) = 0;

public:
    explicit sender_queue(asio::io_context &context) : context(context) {
        timer = create_deadline_timer<int, duration_type>(context);
    }

    void start() {
        std::weak_ptr<sender_queue<value_type, duration_type, allocator>> self(get_shared_from_this());
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

    void set_initial_sequence(size_t s) {
        this->_initial_sequence = s;
    }

    void set_max_sequence(size_type seq) {
        this->_max_sequence = seq;
    }

    size_type get_current_sequence_number() const {
        return this->_initial_sequence;
    }

protected:
    int send_in(const std::shared_ptr<block_type> &b) {
        if (cache.size() < _window_size) {
            return -1;
        }
        b->submit_time_point = get_submit_time();
        cache.push_back(b);
        /// _max_delay后超时
        send(b);
        /// 设置丢包超时
        timer->add_expired_from_now(_max_delay, 1);
        return 1;
    }

    //// 可以为收到ack后，发送seq之后的包
    void try_send_to(size_type seq) {
        auto now = get_submit_time() - _max_delay;
        size_type counts = 0;
        auto it = cache.begin();
        while (it != cache.end()) {
            /// 丢弃已经收到序号的包
            const auto &pkt_seq = (*it)->sequence_number;
            /// 删除比seq小的包
            if (seq >= pkt_seq) {
                cache.erase(it++);
                continue;
            }

            /// 删除可能的回环包
            if (seq < pkt_seq && (pkt_seq - seq >= (_max_sequence >> 1))) {
                cache.erase(it++);
                continue;
            }

            /// 主动丢包
            if (((*it)->submit_time_point) <= now) {
                ++counts;
                cache.erase(it++);
                continue;
            }
            send(*it);
            ++it;
        }
        /// 报告丢包的个数
        on_drop_packet(counts);
    }

private:
    void on_timer(const int &) {
        auto it = cache.begin();
        auto now = get_submit_time() - _max_delay;
        size_type counts = 0;
        while (it != cache.end()) {
            /// 主动丢包
            if (((*it)->submit_time_point) <= now) {
                ++counts;
                cache.erase(it++);
            } else {
                break;
            }
        }
        on_drop_packet(counts);
    }

    inline uint64_t get_submit_time() const {
        auto now = std::chrono::duration_cast<duration_type>(std::chrono::steady_clock::now().time_since_epoch());
        return now.count();
    }

private:
    asio::io_context &context;
    std::shared_ptr<deadline_timer<int, _duration_type>> timer;
    size_type _window_size = 8192;
    size_type _initial_sequence = 0;
    size_type _max_delay = 0;
    size_type _max_sequence = (std::numeric_limits<size_type>::max)();
    std::list<block_type, typename rebind_alloc<block_type>::type> cache;
};


#endif//TOOLKIT_SENDER_BUFFER_HPP
