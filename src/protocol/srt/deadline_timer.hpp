/*
* @file_name: deadline_timer.hpp
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
#ifndef TOOLKIT_DEADLINE_TIMER_HPP
#define TOOLKIT_DEADLINE_TIMER_HPP
#include "spdlog/logger.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <net/asio.hpp>
#include <ratio>
template<typename TAG>
class deadline_timer : public std::enable_shared_from_this<deadline_timer<TAG>> {
    template<typename T>
    friend std::shared_ptr<deadline_timer<T>> create_deadline_timer(asio::io_context &io);

public:
    typedef std::chrono::steady_clock clock_type;
    typedef std::enable_shared_from_this<deadline_timer<TAG>> base_type;
    typedef TAG tag_type;
    typedef typename std::multimap<uint64_t, tag_type>::iterator iterator;
    typedef const iterator const_iterator;

public:
    //// ms,线程不安全
    void add_expired_from_now(uint64_t ms, tag_type tag) {
        std::weak_ptr<deadline_timer<tag_type>> self(base_type::shared_from_this());
        executor.post([self, ms, tag]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now().time_since_epoch()).count();
            stronger_self->triggered_sets.insert(std::make_pair(now + ms, std::move(tag)));
            stronger_self->update_timer();
        });
    }

    void set_on_expired(const std::function<void(const tag_type &)> &f) {
        this->expired_func = f;
    }

    void stop() {
        timer.cancel();
        triggered_sets.clear();
    }

    void update_timer() {
        std::weak_ptr<deadline_timer> self(base_type::shared_from_this());
        /// 移动之
        decltype(triggered_sets) _tmp_ = std::move(triggered_sets);
        auto iter = _tmp_.begin();
        for (; iter != _tmp_.end(); ++iter) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now().time_since_epoch()).count();
            /// expired
            if (static_cast<uint64_t>(now) >= iter->first) {
                /// 这里不能删除，如果回调函数中可能有继续增加任务的操作，迭代器可能会失效
                Info("execute");
                expired_func(std::cref(iter->second));
            } else {
                Info("no triggered");
                /// 如果没有超时.重新恢复
                triggered_sets.insert(iter, _tmp_.end());
                /// 如果为空，直接返回
                if (triggered_sets.empty()) {
                    return;
                }
                std::chrono::milliseconds m(triggered_sets.begin()->first);
                std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long, std::milli>> tp(m);
                timer.cancel();
                timer.expires_at(tp);
                timer.async_wait([self](const std::error_code &e) {
                    if (e) {
                        return;
                    }

                    auto stronger_self = self.lock();
                    if (!stronger_self) {
                        return;
                    }

                    return stronger_self->update_timer();
                });
                break;
            }
        }
    }

    void up_time_to(uint64_t ms) {
        auto it = triggered_sets.upper_bound(ms);
        if (it != triggered_sets.end())
            triggered_sets.erase(triggered_sets.begin(), it);
    }

    const TAG *get_first_element() const {
        static TAG *tag = nullptr;
        if (!triggered_sets.empty()) {
            return &triggered_sets.cbegin();
        }
        return tag;
    }

private:
    explicit deadline_timer(asio::io_context &io) : executor(io), timer(io) {}

private:
    asio::io_context &executor;
    std::recursive_mutex mtx;
    std::function<void(const tag_type &)> expired_func;
    std::multimap<uint64_t, tag_type> triggered_sets;
    asio::steady_timer timer;
};
template<typename T>
std::shared_ptr<deadline_timer<T>> create_deadline_timer(asio::io_context &io) {
    std::shared_ptr<deadline_timer<T>> timer(new deadline_timer<T>(std::ref(io)));
    return timer;
}

#endif//TOOLKIT_DEADLINE_TIMER_HPP
