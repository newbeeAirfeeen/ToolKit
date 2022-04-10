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
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <spdlog/logger.hpp>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/basic_waitable_timer.hpp>
class event_poller : public std::enable_shared_from_this<event_poller> {
public:
    using Ptr = std::shared_ptr<event_poller>;
    using timer_type = asio::basic_waitable_timer<std::chrono::system_clock>;
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
    /*!
     * 执行一个定时任务.切换到自己的polle线程刷新计时器
     * @param duration 多少时间后开始执行,单位为毫秒
     * @param func
     */
    void execute_delay_task(const std::chrono::milliseconds& time, const std::function<size_t()>& func){
         async([this, time, func](){
             this->timer.expires_after(time);
             this->timer.template async_wait([this, func](const std::error_code& er){
                 if(er){
                     Critical(er.message());
                     return;
                 }
                 auto result = func();
                 if(result <= 0)
                     return;
                 this->execute_delay_task(std::chrono::milliseconds(result), func);
             });
         });
    }
    void run();
private:
    asio::io_context _io_context;
    asio::executor_work_guard<typename asio::io_context::executor_type> work_guard;
    /*!
     * 执行_io_context.run()的线程
     */
    std::unique_ptr<std::thread> executor;
    /*!
     * executor所在的线程id
     */
    std::thread::id id;
    /*!
     * 标志是否已经开始运行
     */
    std::atomic<bool> _running;
    std::mutex _mtx_running;
    /*!
     * 定时器
     */
    timer_type timer;
};


#endif//TOOLKIT_EVENT_POLLER_HPP
