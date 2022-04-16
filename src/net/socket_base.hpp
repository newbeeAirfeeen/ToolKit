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
class socket_sender;
template<typename sub_type>
class socket_sender<asio::ip::tcp::socket, sub_type> {
public:
    using clock_type = std::chrono::system_clock;
    using timer_type = asio::basic_waitable_timer<clock_type>;

public:
    socket_sender(asio::ip::tcp::socket &sock, event_poller &poller) : sock(sock), poller(poller), send_timer(poller.get_executor()) {
        this->on_write_done_func = [](size_t length) {};
        this->on_error = [](const std::error_code &e) {};
    }

    virtual std::shared_ptr<sub_type> shared_from_this_subtype() = 0;

    void send(buffer &buf) {
        {
            std::lock_guard<decltype(mtx)> lmtx(mtx);
            L2_cache_.template emplace_back(std::move(buf));
        }
        auto stronger_self = std::static_pointer_cast<socket_sender<asio::ip::tcp::socket, sub_type>>(shared_from_this_subtype());
        return poller.template async([stronger_self]() { stronger_self->write_l(); });
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
        L1_cache_.clear();
        L2_cache_.clear();
    }

    void reset_timer(size_t time) {
        send_timer.expires_after(std::chrono::seconds(time));
    }
private:
    //此函数会在poller绑定的线程执行
    void write_l() {
        auto stronger_self = std::static_pointer_cast<socket_sender<asio::ip::tcp::socket, sub_type>>(shared_from_this_subtype());
        //在自己绑定的线程执行
        if (!stronger_self->is_sending) {
            stronger_self->is_sending = true;
            //如果设置失败
            {
                std::lock_guard<std::recursive_mutex> lmtx(stronger_self->mtx);
                stronger_self->L1_cache_.splice(stronger_self->L1_cache_.end(), stronger_self->L2_cache_);
                if (stronger_self->L1_cache_.empty()) {
                    stronger_self->is_sending = false;
                    return;
                }
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
                    stronger_self->is_sending = false;
                    return stronger_self->on_error(e);
                }
                stronger_self->L1_cache_.remove(length);
                stronger_self->is_sending = false;
                if (stronger_self->L1_cache_.total_buffer_size()) {
                    return stronger_self->write_l();
                }
            };
            stronger_self->sock.async_send(stronger_self->L1_cache_, write_func);
        }
    }

private:
    event_poller &poller;
    std::recursive_mutex mtx;
    asio::ip::tcp::socket &sock;
    timer_type send_timer;
    /**
     * 一级发送缓存
     */
    basic_buffer_list<char> L1_cache_;
    /**
     * 二级发送缓存
     */
    basic_buffer_list<char> L2_cache_;
    std::function<void(const std::error_code &)> on_error;
    std::function<void(size_t)> on_write_done_func;
    /**
     * 是否已经在发送
     */
    bool is_sending = false;
};

template<typename sub_type>
class socket_sender<asio::ip::udp::socket, sub_type> {
public:
    using clock_type = std::chrono::system_clock;
    using timer_type = asio::basic_waitable_timer<clock_type>;
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;

public:
    socket_sender(asio::ip::udp::socket &sock, event_poller &poller) : sock(sock), poller(poller) {
        this->on_write_done_func = [](size_t length) {};
        this->on_error = [](const std::error_code &e, const endpoint_type&) {};
    }

    virtual std::shared_ptr<sub_type> shared_from_this_subtype() = 0;

    bool is_connected() const{
        return this->is_connected_;
    }

    void is_connected(bool connected){
        this->is_connected_ = connected;
    }

    void send(buffer &buf, const endpoint_type &endpoint) {
        auto stronger_self(shared_from_this_subtype());
        std::shared_ptr<buffer> save_buf = std::make_shared<buffer>(std::move(buf));
        auto write_func = [stronger_self, endpoint, save_buf](const std::error_code &e, size_t length) {
            if (stronger_self.unique()) {
                return;
            }
            if (e) {
                return stronger_self->on_error(e, endpoint);
            }

        };
        if(!is_connected())
            return sock.async_send_to(asio::buffer(save_buf->data(), save_buf->size()), endpoint, write_func);
        return sock.async_send(asio::buffer(save_buf->data(), save_buf->size()), write_func);
    }
    /**
     * @description: 设置出错回调
     * @param func
     */
    void setOnErr(const std::function<void(const std::error_code &, const endpoint_type&)> &func) {
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
    }

private:
    event_poller &poller;
    bool is_connected_ = false;
    asio::ip::udp::socket &sock;
    std::function<void(const std::error_code &, const endpoint_type&)> on_error;
    std::function<void(size_t)> on_write_done_func;
};
#endif//TOOLKIT_SOCKET_BASE_HPP
