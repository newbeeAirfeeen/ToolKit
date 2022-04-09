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
#include "buffer.hpp"
#include "event_poller.hpp"
#include "session_base.hpp"
#include "session_helper.hpp"
#include "spdlog/logger.hpp"
#include <Util/nocopyable.hpp>
#ifdef SSL_ENABLE
#include <asio/ssl.hpp>
enum ssl_verify_mode {
    verify_none = asio::ssl::verify_none,
    verify_peer = asio::ssl::verify_peer,
    verify_fail_if_no_peer_cert = asio::ssl::verify_fail_if_no_peer_cert,
    verify_client_once = asio::ssl::verify_client_once,
};
#endif
namespace net {
    template<typename _socket_type>
    class non_ssl_socket : public _socket_type {
    public:
        using socket_type = _socket_type;

    public:
#ifdef SSL_ENABLE
        non_ssl_socket(socket_type &sock, const std::shared_ptr<asio::ssl::context> &context)
            : socket_type(std::move(sock)) {}
#else
        non_ssl_socket(socket_type &sock): socket_type(std::move(sock)) {}
#endif
        template<typename T, typename Func>
        void async_write_some_l(const T& buffer, const Func& func){
            return socket_type::async_write_some(buffer, func);
        }

        template<typename T, typename Func>
        void async_read_some_l(const T& buffer, const Func& func, const std::shared_ptr<basic_session>& session_){
            socket_type::async_read_some(buffer, func);
        }
    };
#ifdef SSL_ENABLE
    template<typename _socket_type>
    class ssl_socket : public _socket_type {
    public:
        using base_type = _socket_type;
        using socket_type = typename _socket_type::next_layer_type;

    public:
        ssl_socket(socket_type &_sock, const std::shared_ptr<asio::ssl::context> &context)
            : base_type(std::move(_sock), *context) {}

        void close() {
            _socket_type::next_layer().close();
        }

        /*!
         * 传this是为了设置已经握手flag, 但需要确保ssl_socket 的子类同时也继承basic_session
         * @param session_ 由子类调用
         */
        template<typename T, typename Func>
        void do_handshake(const T& buffer, const Func& func, const std::shared_ptr<basic_session>& session_) {
            auto handshake_func = [session_, this, buffer, func](const std::error_code &error) {
                if (error) {
                    session_->onError(error);
                    get_session_helper().remove_session(session_);
                    return;
                }
                this->handshake = true;
                this->template async_read_some(buffer, func);
            };
            base_type::async_handshake(asio::ssl::stream_base::server, handshake_func);
        }

        template<typename T, typename Func>
        void async_write_some_l(const T& buffer, const Func& func){
            return base_type::async_write_some(buffer, func);
        }

        template<typename T, typename Func>
        void async_read_some_l(const T& buffer, const Func& func, const std::shared_ptr<basic_session>& session_) {
            if (!handshake)
                do_handshake(buffer, func, session_);
            else base_type::async_read_some(buffer, func);
        }

    private:
        bool handshake = false;
    };
#endif
};// namespace net

template<typename _stream_type>
class session : public basic_session, public _stream_type, public noncopyable {
    friend class tcp_server;
    friend class session_helper;

public:
    using pointer = std::shared_ptr<session>;
    using stream_type = _stream_type;
    using socket_type = typename stream_type::socket_type;

public:
#ifdef SSL_ENABLE
    session(typename stream_type::socket_type &socket_, event_poller &poller, const std::shared_ptr<asio::ssl::context> &context)
        : poller(poller), stream_type(socket_, context), context(context) {}
#else
    session(typename stream_type::socket_type &socket_, event_poller &poller) : poller(poller), stream_type(socket_) {}
#endif
    ~session() {
        Trace("~session");
        stream_type::close();
    }
    event_poller &get_poller() {
        return this->poller;
    }

    virtual void onRecv(const char *data, size_t length) {
        Trace("recv data length {}", length);
        std::string str = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nHello,world";
        send(str);
    }

    virtual void onError(const std::error_code &e) {
        Error(e.message());
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

protected:
    void begin_session() {
        Trace("begin tcp session");
        return this->read_l();
    }

    void read_l() {
        auto stronger_self = std::static_pointer_cast<session<stream_type>>(shared_from_this());
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
        stream_type::async_read_some_l(asio::buffer(stronger_self->buffer, 10240), read_function, session_ptr);
    }

    void write_l() {
        auto stronger_self = std::static_pointer_cast<session<stream_type>>(shared_from_this());
        auto write_function = [stronger_self](const std::error_code &ec, size_t send_length) {
            if (ec) {
                stronger_self->onError(ec);
                get_session_helper().remove_session(stronger_self);
                return;
            }
            stronger_self->_buffer.remove(send_length);
            if (!stronger_self->_buffer.empty()) {
                return stronger_self->write_l();
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
     * ssl上下文
     */
#ifdef SSL_ENABLE
    std::shared_ptr<asio::ssl::context> context;
#endif
};

#endif//TOOLKIT_SESSION_HPP
