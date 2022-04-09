/*
* @file_name: ssl.hpp
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
#if 0
#ifndef TOOLKIT_SSL_HPP
#define TOOLKIT_SSL_HPP
#include "asio.hpp"
#include "session.hpp"
#include "spdlog/logger.hpp"
#include <event_poller.hpp>
#include <memory>
#ifdef SSL_ENABLE
template<typename _session_type, typename = typename std::enable_if<std::is_base_of<basic_session, _session_type>::value>::type>
class ssl : public basic_session {
    friend class tcp_server;

public:
    using session_type = _session_type;
    using context_pointer = std::shared_ptr<asio::ssl::context>;
    using stream_type = typename session_type::stream_type;

public:
    ssl(asio::ip::tcp::socket &sock, event_poller &poller, const context_pointer &ctx_pointer) : session_type(sock, poller), _context(ctx_pointer), stream_socket(session_type::_socket_, *_context) {
        if (!_context)
            throw std::runtime_error("the ssl context is nullptr, please setting valid ssl context with ssl<tcp_session>");
    }

public:
    void begin_session() {
        Trace("ssl begin_session");
        this->read_l();
    }

protected:
    void write_l() {
        auto stronger_self(std::static_pointer_cast<ssl<session_type>>(session_type::shared_from_this()));
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
        stream_socket.template async_write_some(asio::buffer(this->_buffer.data(), this->_buffer.size()), write_function);
    }
    void read_l() {
        auto stronger_self(std::static_pointer_cast<ssl<session_type>>(session_type::shared_from_this()));
        auto read_function = [stronger_self](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                get_session_helper().remove_session(stronger_self);
                return;
            }
            stronger_self->onRecv(stronger_self->buffer, length);
            stronger_self->read_l();
        };
        stream_socket.async_read_some(asio::buffer(stronger_self->buffer, sizeof(this->buffer)),
                                      read_function);
    }

private:
    asio::ssl::stream<stream_type> stream_socket;
    context_pointer _context;
};
#endif
#endif//TOOLKIT_SSL_HPP
#endif