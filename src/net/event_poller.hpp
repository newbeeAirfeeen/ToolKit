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
    /*!
     * 异步执行任务,如果在本线程，则直接执行
     */
    template<typename Func, typename...Args>
    void async(Func&& func, Args&&...args){
        auto current_thread_id = std::this_thread::get_id();
        if(id == current_thread_id){
            func(std::forward<Args>(args)...);
        }
        else{
            auto _func = std::bind(func, std::forward<Args>(args)...);
            _io_context.post(std::move(_func));
        }
    }
    /*!
     * 执行一个定时任务.
     * @param duration 多少时间后开始执行,单位为毫秒
     * @param func
     */
    std::shared_ptr<timer_type> execute_delay_task(const std::chrono::milliseconds& time, const std::function<size_t()>& func){
        std::shared_ptr<timer_type> timer = std::make_shared<timer_type>(get_executor());
        auto self(shared_from_this());
        auto timer_func = std::bind(&event_poller::timer_func_helper, self, std::placeholders::_1, timer, func);
        timer->expires_after(time);
        timer->async_wait(timer_func);
        return timer;
    }

    /*!
     * 创建一个绑定event_poller的定时器
     * @return 定时器
     */
    inline std::shared_ptr<timer_type> create_timer(){
        return std::make_shared<timer_type>(get_executor());
    }

    void run();
private:
    /*!
     * 内部定时器帮助函数
     * @param e 定时器错误码
     * @param timer 定时器
     * @param func 回调函数
     */
    void timer_func_helper(const std::error_code& e, const std::shared_ptr<timer_type>& timer,
                           const std::function<size_t()>& func);
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
};


#endif//TOOLKIT_EVENT_POLLER_HPP
