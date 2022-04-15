/*
* @file_name: socket_base.hpp
* @date: 2022/04/10
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
#ifndef TOOLKIT_SOCKET_BASE_HPP
#define TOOLKIT_SOCKET_BASE_HPP
#include "buffer.hpp"
#include "buffer_list.hpp"
#include <asio.hpp>
#include <chrono>
#include <event_poller.hpp>
#include <functional>
#include <mutex>
#include <utility>
class socket_helper {
public:
    static std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> create_tcp_socket(bool current_thread);
    static std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::udp::socket>> create_udp_socket(bool current_thread);
};


template<typename socket_type, typename sub_type>
class socket_sender {
public:
    using clock_type = std::chrono::system_clock;
    using timer_type = asio::basic_waitable_timer<clock_type>;

public:
    socket_sender(socket_type &sock, event_poller &poller) : sock(sock), poller(poller), send_timer(poller.get_executor()) {
        this->on_write_done_func = [](size_t length) {};
        this->on_error = [](const std::error_code &e) {};
    }

    virtual std::shared_ptr<sub_type> shared_from_this_subtype() = 0;

    void send(buffer &buf) {
        {
            std::lock_guard<decltype(mtx)> lmtx(mtx);
            _L2_cache_.template emplace_back(std::move(buf));
            if (is_sending.load())
                return;
        }
        return write_l();
    }
    /**
     * @description: 设置出错回调
     * @param func
     */
    void setOnErr(const std::function<void(const std::error_code &)> &func) {
        this->on_error = func;
    }
    /**
     * @description: 设置写完毕回调
     * @param func
     */
    void setWriteDone(const std::function<void(size_t)> &func) {
        this->on_write_done_func = func;
    }

public:
    virtual size_t get_send_time_out() {
        return 0;
    }

    void clear() {
        sock.cancel();
        _L1_cache_.clear();
        _L2_cache_.clear();
    }

protected:
    void reset_timer(size_t time) {
        send_timer.expires_after(std::chrono::seconds(time));
    }

private:
    void write_l() {
        auto stronger_self = std::static_pointer_cast<socket_sender<socket_type, sub_type>>(shared_from_this_subtype());
        auto send_result = false;
        if (is_sending.compare_exchange_weak(send_result, true)) {
            poller.async([stronger_self]() {
                {
                    std::lock_guard<std::recursive_mutex> lmtx(stronger_self->mtx);
                    stronger_self->_L1_cache_.splice(stronger_self->_L1_cache_.end(), stronger_self->_L2_cache_);
                }
                if (stronger_self->_L1_cache_.empty()) {
                    stronger_self->is_sending.store(false, std::memory_order_relaxed);
                    return;
                }
                auto time_out = stronger_self->get_send_time_out();
                if (time_out) {
                    auto origin_time_out = clock_type::now() + std::chrono::seconds(time_out);
                    stronger_self->send_timer.expires_at(origin_time_out);
                    stronger_self->send_timer.template async_wait([stronger_self](const std::error_code &e) {
                        //此时只剩定时器持有引用
                        if (stronger_self.unique() || e) {
                            return;
                        }
                        Error("session send timeout");
                        stronger_self->sock.shutdown(asio::socket_base::shutdown_both);
                    });
                }
                auto write_func = [stronger_self](const std::error_code &e, size_t length) {
                    if (stronger_self.unique()) {
                        return;
                    }
                    if (e) {
                        return stronger_self->on_error(e);
                    }
                    stronger_self->_L1_cache_.remove(length);
                    if (stronger_self->_L1_cache_.total_buffer_size()) {
                        return stronger_self->write_l();
                    }
                    stronger_self->is_sending.store(false, std::memory_order::memory_order_release);
                };
                stronger_self->sock.async_send(stronger_self->_L1_cache_, write_func);
            });
        }
    }

private:
    event_poller &poller;
    std::recursive_mutex mtx;
    socket_type &sock;
    timer_type send_timer;
    /**
     * 一级发送缓存
     */
    basic_buffer_list<char> _L1_cache_;
    /**
     * 二级发送缓存
     */
    basic_buffer_list<char> _L2_cache_;
    std::function<void(const std::error_code &)> on_error;
    std::function<void(size_t)> on_write_done_func;
    /**
     * 是否已经在发送
     */
    std::atomic<bool> is_sending{false};
};

#endif//TOOLKIT_SOCKET_BASE_HPP
