/*
* @file_name: executor.cpp
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
#include "executor.hpp"
#include "spdlog/logger.hpp"
#include <chrono>
executor::~executor() {
    stop();
    if (_executor && _executor->joinable()) {
        _executor->join();
    }
}

const std::thread::id &executor::get_thread_id() const {
    return this->_id;
}

void executor::start() {
    int _ = 0;
    if (!is_start.compare_exchange_strong(_, 1)) {
        return;
    }
    Trace("wait to start...");
    _executor.reset(new std::thread([this]() {
        this->run();
    }));
    _ = 1;
    while (!is_start.compare_exchange_weak(_, 2)) {
        _ = 1;
    }
    Trace("start success");
}

void executor::stop() {
    this->async([]() {
        throw std::bad_function_call();
    });
}

void executor::wait() {
    if (_executor && _executor->joinable()) {
        _executor->join();
    }
}
void executor::run() {
    try {
        this->_id = std::this_thread::get_id();
        is_start.store(1);
        while (true) {
            decltype(funcs) _tmp_funcs;
            {
                std::unique_lock<std::mutex> lmtx(mtx);
                while (funcs.empty())
                    cv.wait(lmtx);
                _tmp_funcs = std::move(funcs);
            }
            while (!_tmp_funcs.empty()) {
                auto f = std::move(_tmp_funcs.front());
                if (f) {
                    Trace("execute task...");
                    f();
                }
                _tmp_funcs.pop_front();
            }
        }
    } catch (const std::exception &) {
    }
    is_start.store(0);
}