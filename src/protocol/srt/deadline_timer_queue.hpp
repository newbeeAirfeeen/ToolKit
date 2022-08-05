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
    friend std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(asio::io_context &ex);

public:
    typedef asio::io_context executor_type;
    typedef Key key_type;
    typedef std::pair<uint64_t, Value> value_type;
    typedef std::enable_shared_from_this<deadline_timer_queue<Key, Value>> base_type;
    typedef std::chrono::system_clock clock_type;
public:
    void set_deliver_duration(uint64_t deliver_duration) {
        this->deliver_duration = deliver_duration;
    }

    void enqueue(Key&& key, value_type&& value) {

    }


    void set_on_deliver(const std::function<void()>&) {
    }

private:
    deadline_timer_queue(const std::chrono::milliseconds &m, executor_type &ex) :executor(ex) {}
private:
    std::map<key_type, value_type> queue_cache;
    executor_type &executor;
    uint64_t deliver_duration = 0;
};

template<typename Key, typename Value>
std::shared_ptr<deadline_timer_queue<Key, Value>> create_deadline_timer_queue(asio::io_context &ex) {
    std::shared_ptr<deadline_timer_queue<Key, Value>> queue(new deadline_timer_queue<Key, Value>(std::ref(ex)));
    return queue;
}

#endif//TOOLKIT_DEADLINE_TIMER_QUEUE_HPP
