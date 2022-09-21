/*
* @file_name: executor_pool.cpp
* @date: 2022/09/07
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

#include "executor_pool.hpp"
#include <atomic>

size_t executor_pool::num = 1;
executor_pool &executor_pool::instance() {
    static executor_pool pool;
    return pool;
}

void executor_pool::set_nums(size_t num) {
    executor_pool::num = num;
}
executor_pool::executor_pool() {
    for (decltype(num) i = 0; i < num; i++) {
        auto executor_ = std::make_shared<executor>();
        executor_->start();
        executors.push_back(executor_);
    }
}


std::shared_ptr<executor> executor_pool::get_executor() {
    std::shared_ptr<executor> executor_;
    std::shared_ptr<std::atomic<char>> _load = std::make_shared<std::atomic<char>>(0);
    std::weak_ptr<std::atomic<char>> self(_load);
    for_each([&, self](const std::shared_ptr<executor> &ev_ptr) {
        auto stronger_self = self.lock();
        if (!stronger_self) {
            return;
        }

        char _0 = 0;
        if (!stronger_self->compare_exchange_strong(_0, 1)) {
            return;
        }

        executor_ = ev_ptr;
        stronger_self->store(2);
    });
    char _ = 2;
    while (!_load->compare_exchange_weak(_, 3)) {
        _ = 2;
    }
    return executor_;
}