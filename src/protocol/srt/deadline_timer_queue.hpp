/*
* @file_name: dealine_timer_queue.hpp
* @date: 2022/08/05
* @author: oaho
* Copyright @ hz oaho, All rights reserved.
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
#include "deadline_timer.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <net/asio.hpp>

template<typename Key, typename Value>
class deadline_timer_queue : public std::enable_shared_from_this<deadline_timer_queue<Key, Value>> {
    friend std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(const std::chrono::milliseconds &, asio::io_context &ex);

public:
    typedef asio::io_context executor_type;
    typedef Key key_type;
    typedef std::pair<uint64_t, Value> value_type;
    typedef std::enable_shared_from_this<deadline_timer_queue<Key, Value>> base_type;
    typedef std::chrono::system_clock clock_type;
public:
    void set_deliver_duration(const std::chrono::milliseconds &m) {
        std::weak_ptr<deadline_timer_queue<key_type, value_type>> self(base_type::shared_from_this());
        executor.post([self, m]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            stronger_self->deadline_duration = m;
        });
    }

    void enqueue(Key&& key, value_type&& value) {
        std::weak_ptr<deadline_timer_queue<key_type, value_type>> self(base_type::shared_from_this());
        auto insert_time = clock_type::now().time_since_epoch().count();
        uint64_t expired_time = clock_type::now().time_since_epoch().count() + deadline_duration.count();
        executor.post([self, expired_time, insert_time]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            auto now = clock_type::now();

        });
    }

    template<typename F, typename... Args>
    void set_on_deliver(F &&f, Args &&...args) {
    }

private:
    deadline_timer_queue(const std::chrono::milliseconds &m, executor_type &ex) : deadline_duration(m), executor(ex), deadline_timer(ex) {}
private:
    std::map<key_type, value_type> queue_cache;
    asio::system_timer deadline_timer;
    executor_type &executor;
    std::chrono::milliseconds deadline_duration;
};

template<typename Key, typename Value>
std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(const std::chrono::milliseconds &m, asio::io_context &ex) {
    std::shared_ptr<deadline_timer_queue<Key, Value>> queue(new deadline_timer_queue<Key, Value>(m, std::ref(ex)));
    return queue;
}

#endif//TOOLKIT_DEADLINE_TIMER_QUEUE_HPP
