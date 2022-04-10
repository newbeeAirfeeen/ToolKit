/*
* @file_name: session.hpp
* @date: 2022/04/06
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

#ifndef TOOLKIT_SESSION_HPP
#define TOOLKIT_SESSION_HPP
#include "asio/basic_waitable_timer.hpp"
#include "buffer.hpp"
#include "event_poller.hpp"
#include "session_base.hpp"
#include "session_helper.hpp"
#include "spdlog/logger.hpp"
#ifdef SSL_ENABLE
#include "asio/ssl.hpp"
#endif
#include <Util/nocopyable.hpp>
template<typename _stream_type>
class session : public basic_session, public _stream_type, public noncopyable {
    friend class tcp_server;
    friend class session_helper;

public:
    using pointer = std::shared_ptr<session>;
    using stream_type = _stream_type;
    using socket_type = typename stream_type::socket_type;
    using clock_type  = std::chrono::system_clock;
    using timer = asio::basic_waitable_timer<clock_type>;

public:
#ifdef SSL_ENABLE
    session(typename stream_type::socket_type &socket_, event_poller &poller,
            const std::shared_ptr<asio::ssl::context> &context)
        : poller(poller), stream_type(socket_, context),
          context(context), recv_timer(poller.get_executor()),
          send_timer(poller.get_executor()) {}

    session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>>& pair,
            const std::shared_ptr<asio::ssl::context> &context)
        :poller(*pair.first), stream_type(*pair.second, context),
         context(context), recv_timer(pair.first->get_executor()), send_timer(pair.first->get_executor()){}
#else
    session(typename stream_type::socket_type &socket_, event_poller &poller)
        : poller(poller), stream_type(socket_), recv_timer(poller.get_executor()),
          send_timer(poller.get_executor()) {}

    session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>>& pair)
        :poller(*pair.first), stream_type(*pair.second),
          recv_timer(pair.first->get_executor()), send_timer(pair.first->get_executor()){}
#endif
    ~session() {
        Trace("~session");
        stream_type::close();
    }
    event_poller &get_poller() {
        return this->poller;
    }
    /*!
     * 为数据接收回调
     * @param data 数据头指针
     * @param length 数据的长度
     */
    void onRecv(const char *data, size_t length)override {
        Trace("recv data length {}", length);
    }

    /*!
     * 会话出错回调
     * @param e 错误码
     */
    virtual void onError(const std::error_code &e) override {
        Error(e.message());
    }
    /*!
     * 在会话开始前会被调用
     */
    virtual void before_begin_session(){}
    /*!
     * 设置发送超时, 0为没有超时时间
     * @param time 时间单位为毫秒
     */
    void set_send_time_out(size_t time) {
        send_time_out.store(time);
        this->send_timer.expires_after(time);
    }
    /*!
     * 设置接收时间超时,0为没有超时时间
     * @param time 时间单位为毫秒
     */
    void set_recv_time_out(size_t time) {
        recv_time_out.store(time);
        this->recv_timer.expires_at(std::chrono::seconds(time));
    }
    /*!
     * 设置接收缓冲区大小
     * @param size 接收缓冲区的大小
     */
    void set_recv_buffer_size(size_t size) {
        return stream_type::set_recv_buffer_size(size);
    }
    /*!
     * 设置发送缓冲区的大小
     * @param size 发送缓冲区的大小
     */
    void set_send_buffer_size(size_t size) {
        return stream_type::set_send_buffer_size(size);
    }
    /*!
     * 设置发送低水位
     * @param size 设置发送低水位
     */
    void set_send_low_water_mark(size_t size) {
        return stream_type::set_send_low_water_mark(size);
    }
    /*!
     * 设置接收低水位
     * @param 接收低水位
     */
    void set_recv_low_water_mark(size_t size) {
        return stream_type::set_recv_low_water_mark(size);
    }
    /*!
     * 是否关闭nagle算法
     * @param no_delay true关闭,false开启。
     */
    void set_no_delay(bool no_delay = true) {
        return stream_type::set_no_delay(no_delay);
    }

protected:
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param buffer buffer
     */
    void send(basic_buffer<char> &buffer) {
        if (_buffer.empty())
            _buffer.swap(buffer);
        else
            _buffer.append(buffer.data(), buffer.size());
        return this->write_l();
    }
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param data 字符头指针
     * @param length 字节数据长度
     */
    void send(const char *data, size_t length) {
        _buffer.append(data, length);
        return this->write_l();
    }
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param str basic_string
     */
    void send(std::string &str) {
        if (_buffer.empty()) {
            decltype(_buffer) tmp_buffer(std::move(str));
            _buffer.swap(tmp_buffer);
        } else
            _buffer.append(str.data(), str.size());
        return this->write_l();
    }

public:
    virtual void begin_session() {
        Trace("begin tcp session");
        before_begin_session();
        return this->read_l();
    }
public:
    void read_l() {
        auto stronger_self = std::static_pointer_cast<session<stream_type>>(shared_from_this());
        auto time_out = recv_time_out.load(std::memory_order_relaxed);
        if (time_out) {
            auto origin_time_out = clock_type::now() + std::chrono::seconds(time_out);
            recv_timer.expires_at(origin_time_out);
            recv_timer.template async_wait([stronger_self, origin_time_out](const std::error_code &e) {
                Trace(e.message());
                //此时只剩定时器持有引用
                if (stronger_self.unique() || e) {
                    return;
                }
                Error("session receive timeout");
                stronger_self->stream_type::shutdown(0);
            });
        }
        auto read_function = [stronger_self](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                get_session_helper().remove_session(stronger_self);
                return;
            }
            stronger_self->onRecv(stronger_self->buffer, length);
            stronger_self->read_l();
        };
        std::shared_ptr<basic_session> session_ptr(shared_from_this());
        stream_type::async_read_some_l(asio::buffer(stronger_self->buffer, 10240), read_function, session_ptr, true);
    }

    void write_l() {
        auto stronger_self = std::static_pointer_cast<session<stream_type>>(shared_from_this());
        auto time_out = send_time_out.load(std::memory_order_relaxed);
        if (time_out) {
            auto origin_time_out = clock_type::now() + std::chrono::seconds(time_out);
            send_timer.expires_at(origin_time_out);
            send_timer.template async_wait([stronger_self](const std::error_code &e) {
                //此时只剩定时器持有引用
                if (stronger_self.unique() || e) {
                    return;
                }
                Error("session send timeout");
                stronger_self->stream_type::shutdown(1);
            });
        }
        auto write_function = [stronger_self](const std::error_code &ec, size_t send_length) {
            if (ec) {
                stronger_self->stream_type::shutdown(0);
                return;
            }
            stronger_self->_buffer.remove(send_length);
            if (!stronger_self->_buffer.empty()) {
                return stronger_self->write_l();
            }
            else{
                //如果没有数据可发，取消定时器
                stronger_self->send_timer.cancel();
            }
        };
        stream_type::async_write_some_l(asio::buffer(_buffer.data(), _buffer.size()), write_function);
    }

protected:
    /*!
     * 用户读缓冲区
     */
    char buffer[10240] = {0};
    event_poller &poller;
    /*!
     * 用户写缓冲区
     */
    basic_buffer<char> _buffer;
    /*!
     * 超时定时器
     */
    timer recv_timer;
    timer send_timer;
    /*!
     * 发送时间超时
     */
    std::atomic_size_t send_time_out{15};
    /*!
     * 接收时间超时
     */
    std::atomic_size_t recv_time_out{15};
    /*!
     * ssl上下文
     */
#ifdef SSL_ENABLE
    std::shared_ptr<asio::ssl::context> context;
#endif
};

#endif//TOOLKIT_SESSION_HPP
