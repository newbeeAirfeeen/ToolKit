/*
* @file_name: executor.hpp
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

#ifndef TOOLKIT_TASK_EXECUTOR_HPP
#define TOOLKIT_TASK_EXECUTOR_HPP
#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
class executor : public std::enable_shared_from_this<executor> {
public:
    ~executor();
    const std::thread::id &get_thread_id() const;

    template<typename FUNC, typename... Args>
    auto async(FUNC &&F, Args &&...args) -> void {
        auto id = std::this_thread::get_id();
        if (id == _id) {
            F(std::forward<Args>(args)...);
            return;
        }

        auto func = std::bind(F, std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lmtx(mtx);
        funcs.emplace_back(func);
        cv.notify_one();
    }

    void start();
    void stop();
    void wait();
private:
    void run();

private:
    std::mutex mtx;
    std::condition_variable cv;
    std::unique_ptr<std::thread> _executor;
    std::list<std::function<void()>> funcs;
    std::thread::id _id;
    std::atomic<int> is_start{0};
};


#endif//TOOLKIT_TASK_EXECUTOR_HPP
