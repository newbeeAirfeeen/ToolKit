/*
* @file_name: event_poller.cpp
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
#include "event_poller.hpp"
#include "Util/onceToken.h"
#include "asio/executor_work_guard.hpp"
#include "spdlog/logger.hpp"
class thread_quit_exception : public std::exception {};

event_poller::event_poller() : _running(false) {
}

event_poller::~event_poller() {
    stop();
    Debug("event_poller 销毁");
}

void event_poller::start() {
    std::lock_guard<std::mutex> lmtx(_mtx_running);
    if (_running.load(std::memory_order_acquire)) {
        return;
    }
    executor.reset(new std::thread([this] {
        this->run();
    }));
    bool _ = true;
    while(!_running.compare_exchange_weak(_, true)){
        _ = true;
    }
}

void event_poller::stop() {
    async([]() {
        throw thread_quit_exception();
    });
    if (executor && executor->joinable())
        executor->join();
}

void event_poller::join() {
    if (executor && executor->joinable()) {
        executor->join();
    }
}

asio::io_context &event_poller::get_executor() {
    return this->_io_context;
}

const std::thread::id &event_poller::get_thread_id() const {
    return this->id;
}

void event_poller::run() {
    Trace("event_poller 启动");
    this->id = std::this_thread::get_id();
    auto construct_func = [&] { this->_running.store(true, std::memory_order_relaxed); };
    auto destroy_func = [&] { this->_running.store(false, std::memory_order_relaxed); };
    toolkit::onceToken token(construct_func, destroy_func);
    while (true) {
        try {
            asio::executor_work_guard<typename asio::io_context::executor_type> work_guard(_io_context.get_executor());
            _io_context.run();
        } catch (const thread_quit_exception &) {
            break;
        } catch (const std::exception &e) {
            Error(e.what());
        }
    }
}

void event_poller::timer_func_helper(const std::error_code &e, const std::shared_ptr<timer_type> &timer, const std::function<size_t()> &func) {
    if (e) {
        Debug(e.message());
        return;
    }
    auto result = func();
    if (result <= 0) {
        return;
    }
    auto timer_func = std::bind(&event_poller::timer_func_helper, shared_from_this(), std::placeholders::_1, timer, func);
    timer->expires_after(std::chrono::milliseconds(result));
    timer->async_wait(timer_func);
}