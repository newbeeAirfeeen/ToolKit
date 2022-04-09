/*
* @file_name: EventPoller.hpp
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
#ifndef TOOLKIT_EVENT_POLLER_HPP
#define TOOLKIT_EVENT_POLLER_HPP
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <memory>
#include <thread>
#include <atomic>
class event_poller : public std::enable_shared_from_this<event_poller> {
public:
    using Ptr = std::shared_ptr<event_poller>;
public:
    event_poller();

    ~event_poller();

    /*!
     * 启动一个poller
     */
    void start();
    /*!
     * 停止运行
     */
    void stop();

    /*!
     * 等待内部线程退出
     */
    void join();
    /*!
     * 得到当前poller中的绑定的io_context上下文
     * @return
     */
    asio::io_context& get_executor();
    /*!
     * 返回当前event_poller的线程id
     * @return 当前线程的id
     */
    const std::thread::id& get_thread_id() const;

    template<typename Func, typename...Args>
    void async(Func&& func, Args&&...args){
        auto _func = std::bind(func, std::forward<Args>(args)...);
        _io_context.template post(std::move(_func));
    }
protected:
    void run();
private:
    asio::io_context _io_context;
    asio::executor_work_guard<typename asio::io_context::executor_type> work_guard;
    std::unique_ptr<std::thread> executor;
    std::thread::id id;
    std::atomic<bool> _running;
    std::mutex _mtx_running;
};


#endif//TOOLKIT_EVENT_POLLER_HPP
