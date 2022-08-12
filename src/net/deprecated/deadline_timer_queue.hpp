/*
* @file_name: dealine_timer_queue.hpp
* @date: 2022/08/05
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

#ifndef TOOLKIT_DEADLINE_TIMER_QUEUE_HPP
#define TOOLKIT_DEADLINE_TIMER_QUEUE_HPP
#include "protocol/srt/deadline_timer.hpp"
#include <functional>
#include <map>
#include <memory>
#include "net/asio.hpp"
#include <utility>
template<typename Key, typename Value>
class deadline_timer_queue : public std::enable_shared_from_this<deadline_timer_queue<Key, Value>> {
    friend std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(asio::io_context &ex);

public:
    typedef asio::io_context executor_type;
    typedef Key key_type;
    typedef std::pair<uint64_t, Value> value_type;
    typedef std::enable_shared_from_this<deadline_timer_queue<Key, Value>> base_type;
    typedef std::chrono::system_clock clock_type;
    typedef std::function<void(const key_type &, const value_type &)> deliver_callback_type;

public:
    void set_deliver_duration(size_t d) {
        this->deliver_duration = d;
    }

    void enqueue(Key &&key, value_type &&value) {
    }

    void set_max_size(size_t size) {
        this->que_max_size = size;
    }

    void set_on_deliver(const deliver_callback_type &f) {
        this->on_out_of_queue = f;
    }

    void set_out_of_queue(const deliver_callback_type &f) {
        this->on_deliver = f;
    }

    void set_enable_drop_too_late(bool on) {
        this->_enable_drop_too_late = on;
    }

    /// 小于这个时间戳的都会被drop, ms 单位
    void update_time_from_now() {
        std::weak_ptr<deadline_timer_queue<key_type, value_type>> self(base_type::shared_from_this());
        executor.post([self]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            /// 得到当前的时间，小于 当前这个时间点 - deliver_duration 的都会被丢弃
            auto deadline = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now().time_since_epoch()).count() - stronger_self->deliver_duration;
            /// 移除时间点之前的所有元素
            stronger_self->timer.up_time_to(deadline);
            auto *first_element = stronger_self->timer.get_first_element();
            /// 如果为空, 则清空所有包
            if (!first_element) {
                stronger_self->queue_cache.clear();
                return;
            }
            /// 找到时间点之前的所有序号
            auto iter = stronger_self->queue_cache.upper_bound(first_element->second.first);
            if (iter == stronger_self->queue_cache.end()) {
                return;
            }
            /// 删除对应序号的包
            stronger_self->queue_cache.erase(stronger_self->queue_cache.begin(), iter);
            /// 更新定时器
            stronger_self->timer.update_timer();
        });
    }

private:
    deadline_timer_queue(const std::chrono::milliseconds &m, executor_type &ex) : executor(ex), timer(ex) {}

private:
    /// 包缓冲区
    std::map<key_type, value_type> queue_cache;
    executor_type &executor;
    /// 包递送间隔
    size_t deliver_duration = 100;
    /// 队列最大容量
    size_t que_max_size = 8192;
    /// 包溢出回调
    deliver_callback_type on_out_of_queue;
    /// 递送回调
    deliver_callback_type on_deliver;
    deadline_timer<std::pair<key_type, value_type>> timer;
    bool _enable_drop_too_late = true;
};

template<typename Key, typename Value>
std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(asio::io_context &ex) {
    std::shared_ptr<deadline_timer_queue<Key, Value>> queue(new deadline_timer_queue<Key, Value>(std::ref(ex)));
    return queue;
}

#endif//TOOLKIT_DEADLINE_TIMER_QUEUE_HPP
