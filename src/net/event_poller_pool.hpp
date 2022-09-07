/*
* @file_name: EventPollerPool.hpp
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
#ifndef TOOLKIT_EVENT_POLLER_POOL_HPP
#define TOOLKIT_EVENT_POLLER_POOL_HPP
#include "Util/nocopyable.hpp"
#include "event_poller.hpp"
#include <thread>
#include <vector>
class event_poller_pool : public noncopyable {
public:
    ~event_poller_pool();

public:
    static event_poller_pool &Instance();

public:
    event_poller::Ptr get_poller(bool current_thread = true);
    /*!
     * 当前线程的数量
     */
    size_t size() const;
    /*!
     * 广播所有线程执行任务
     * @tparam Func 可执行函数对象
     * @tparam Args 任意个数的参数
     * @param f
     * @param args
     */
    void for_each(const std::function<void(const event_poller::Ptr &)> &f);

    void wait();

private:
    explicit event_poller_pool(size_t num = std::thread::hardware_concurrency());

private:
    std::vector<event_poller::Ptr> poller_pool;
    size_t _num;
};


#endif//TOOLKIT_EVENT_POLLER_POOL_HPP
