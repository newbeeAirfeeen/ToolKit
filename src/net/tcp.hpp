/*
* @file_name: tcp.hpp
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
#ifndef TOOLKIT_TCP_HPP
#define TOOLKIT_TCP_HPP
#include <asio.hpp>
#ifdef SSL_ENABLE
#include <asio/ssl.hpp>
enum ssl_verify_mode {
    verify_none = asio::ssl::verify_none,
    verify_peer = asio::ssl::verify_peer,
    verify_fail_if_no_peer_cert = asio::ssl::verify_fail_if_no_peer_cert,
    verify_client_once = asio::ssl::verify_client_once,
};
#endif
template<typename _socket_type>
class non_ssl: public _socket_type {
public:
    using socket_type = _socket_type;

public:
#ifdef SSL_ENABLE
    non_ssl(socket_type &sock, const std::shared_ptr<asio::ssl::context> &context)
        : socket_type(std::move(sock)) {}
#else
    non_ssl(socket_type &sock) : socket_type(std::move(sock)) {}
#endif
    template<typename T, typename Func>
    void async_write_some_l(const T &buffer, const Func &func) {
        return socket_type::async_write_some(buffer, func);
    }

    template<typename T, typename Func>
    void async_read_some_l(const T &buffer, const Func &func, const std::shared_ptr<basic_session> &session_) {
        socket_type::async_read_some(buffer, func);
    }

    void set_recv_buffer_size(size_t size) {
        asio::ip::tcp::socket::receive_buffer_size recv_buf_size(size);
        socket_type::set_option(recv_buf_size);
    }

    void set_send_buffer_size(size_t size) {
        asio::ip::tcp::socket::send_buffer_size send_buf_size(size);
        socket_type::set_option(send_buf_size);
    }

    void set_send_low_water_mark(size_t size) {
        asio::ip::tcp::socket::send_low_watermark send_low_wtmark(size);
        socket_type::set_option(send_low_wtmark);
    }

    void set_recv_low_water_mark(size_t size) {
        asio::ip::tcp::socket::receive_low_watermark recv_low_wtmark(size);
        socket_type::set_option(recv_low_wtmark);
    }

    void set_no_delay(bool no_delay) {
        asio::ip::tcp::no_delay no_delay_(no_delay);
        socket_type::set_option(no_delay_);
    }
};
#ifdef SSL_ENABLE
template<typename _socket_type>
class ssl : public _socket_type {
public:
    using base_type = _socket_type;
    using socket_type = typename _socket_type::next_layer_type;

public:
    ssl(socket_type &_sock, const std::shared_ptr<asio::ssl::context> &context)
        : base_type(std::move(_sock), *context) {}

    void close() {
        _socket_type::next_layer().close();
    }

    /*!
     * 传this是为了设置已经握手flag, 但需要确保ssl_socket 的子类同时也继承basic_session
     * @param session_ 由子类调用
     */
    template<typename T, typename Func>
    void do_handshake(const T &buffer, const Func &func, const std::shared_ptr<basic_session> &session_) {
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
    void async_write_some_l(const T &buffer, const Func &func) {
        return base_type::async_write_some(buffer, func);
    }

    template<typename T, typename Func>
    void async_read_some_l(const T &buffer, const Func &func, const std::shared_ptr<basic_session> &session_) {
        if (!handshake)
            do_handshake(buffer, func, session_);
        else
            base_type::async_read_some(buffer, func);
    }

    void set_recv_buffer_size(size_t size) {
        asio::ip::tcp::socket::receive_buffer_size recv_buf_size(size);
        base_type::next_layer().set_option(recv_buf_size);
    }

    void set_send_buffer_size(size_t size) {
        asio::ip::tcp::socket::send_buffer_size send_buf_size(size);
        base_type::next_layer().set_option(send_buf_size);
    }

    void set_send_low_water_mark(size_t size) {
        asio::ip::tcp::socket::send_low_watermark send_low_wtmark(size);
        base_type::next_layer().set_option(send_low_wtmark);
    }

    void set_recv_low_water_mark(size_t size) {
        asio::ip::tcp::socket::receive_low_watermark recv_low_wtmark(size);
        base_type::next_layer().set_option(recv_low_wtmark);
    }

    void set_no_delay(bool no_delay) {
        asio::ip::tcp::no_delay no_delay_(no_delay);
        base_type::next_layer().set_option(no_delay_);
    }

private:
    bool handshake = false;
};
#endif
#endif//TOOLKIT_TCP_HPP
