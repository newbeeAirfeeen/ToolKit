/*
* @file_name: EventPollerPool.cpp
* @date: 2022/04/04
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
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "event_poller_pool.hpp"
event_poller_pool &event_poller_pool::Instance() {
    auto num = (std::thread::hardware_concurrency() / 2) & 0xFFFFFFFE;
    static event_poller_pool pool(num ? num : 1);
    return std::ref(pool);
}

event_poller_pool::~event_poller_pool() {
    for (auto &poller: poller_pool) {
        poller->stop();
    }
}

event_poller_pool::event_poller_pool(size_t num) : _num(num) {
    for (size_t i = 0; i < _num; i++) {
        auto poller = std::make_shared<event_poller>();
        poller->start();
        poller_pool.emplace_back(poller);
    }
}

event_poller::Ptr event_poller_pool::get_poller(bool current_thread) {
    if (current_thread) {
        auto it = std::find_if(poller_pool.begin(),
                               poller_pool.end(), [](const event_poller::Ptr &ev_ptr) -> bool {
                                   return std::this_thread::get_id() == ev_ptr->get_thread_id();
                               });
        if (it != poller_pool.end()) {
            return *it;
        }
    }
    event_poller::Ptr pointer;
    std::shared_ptr<std::atomic<char>> _load = std::make_shared<std::atomic<char>>(0);
    std::weak_ptr<std::atomic<char>> self(_load);
    std::for_each(poller_pool.begin(), poller_pool.end(), [&, self](const event_poller::Ptr ev_ptr) {
        auto stronger_self = self.lock();
        if (!stronger_self) {
            return;
        }

        char _0 = 0;
        if (!stronger_self->compare_exchange_strong(_0, 1)) {
            return;
        }

        pointer = ev_ptr;
        stronger_self->store(2);
    });
    char _ = 2;
    while (!_load->compare_exchange_weak(_, 2)) {
        _ = 2;
    }
    return pointer;
}

size_t event_poller_pool::size() const {
    return this->_num;
}

void event_poller_pool::for_each(const std::function<void(const event_poller::Ptr &)> &f) {
    for (auto &poller: poller_pool) {
        auto func = std::bind(f, poller);
        poller->async(func);
    }
}


void event_poller_pool::wait() {
    for (auto &poller: poller_pool) {
        poller->join();
    }
}