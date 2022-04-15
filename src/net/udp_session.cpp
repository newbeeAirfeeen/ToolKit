/*
* @file_name: udp_session.cpp
* @date: 2022/04/13
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
#include "udp_session.hpp"
#include "spdlog/logger.hpp"
#include "udp_server.hpp"
#ifdef SSL_ENABLE

udp_session::udp_session(event_poller &poller, const std::shared_ptr<context> &context_, asio::ip::udp::socket &sock)
    : poller(poller), _context(context_), recv_timer(poller.get_executor()), sock(sock),udp_helper(sock),
      socket_sender<asio::ip::udp::socket, udp_session>(sock, poller) {
}

#else
udp_session::udp_session(event_poller &poller, asio::ip::udp::socket &sock)
    : poller(poller), recv_timer(poller.get_executor()), sock(sock),
      udp_helper(sock),socket_sender<asio::ip::udp::socket, udp_session>(sock, poller) {
}
#endif

udp_session::~udp_session() {
    Info("~udp_session");
}

std::shared_ptr<udp_session> udp_session::shared_from_this_subtype() {
    return std::static_pointer_cast<udp_session>(shared_from_this());
}

void udp_session::onRecv(const char *data, size_t size) {
    Info("udp recv {} length, {}", size, data);
    basic_buffer<char> b(data, size);
    send(b);
}

void udp_session::onError(const std::error_code &e) {
    Error(e.message());
}

void udp_session::send(basic_buffer<char> &buffer) {
    if (buffer_.empty()) {
        buffer_.swap(buffer);
    } else {
        buffer_.append(buffer.data(), buffer.size());
    }
    return write_l();
}

void udp_session::launchRecv(basic_buffer<char> &buff, const endpoint_type &endpoint) {
    std::shared_ptr<basic_buffer<char>> tmp_buffer = std::make_shared<basic_buffer<char>>(std::move(buff));
    std::weak_ptr<udp_session> self(std::static_pointer_cast<udp_session>(shared_from_this()));
    poller.async([self, tmp_buffer, endpoint]() {
        auto stronger_self = self.lock();
        if (!stronger_self) {
            return;
        }
        stronger_self->endpoint = endpoint;
        stronger_self->onRecv(tmp_buffer->data(), tmp_buffer->size());
    });
}

void udp_session::write_l() {
    auto stronger_self(std::static_pointer_cast<udp_session>(shared_from_this()));
    auto write_func = [stronger_self](const std::error_code &e, size_t length) {
        if (stronger_self.unique())
            return;
        if (e) {
            Error(e.message());
            return;
        }
        stronger_self->buffer_.remove(length);
        if (stronger_self->buffer_.size()) {
            return stronger_self->write_l();
        }
    };
    return sock.async_send_to(
            asio::buffer(stronger_self->buffer_.data(),
                         stronger_self->buffer_.size()),
            stronger_self->endpoint, write_func);
}

using endpoint_type = typename udp_session::endpoint_type;

void udp_session::attach_server(udp_server *server) {
    this->server = server;
}

void udp_session::begin_session() {
    auto time_out_ = time_out.load(std::memory_order_relaxed);
    if (time_out_) {
        auto stronger_self(std::static_pointer_cast<udp_session>(shared_from_this()));
        recv_timer.expires_after(std::chrono::seconds(time_out_));
        recv_timer.async_wait([stronger_self](const std::error_code &e) {
            Trace(e.message());
            //此时只剩定时器持有引用
            if (stronger_self.unique() || e) {
                return;
            }
            Error("session receive timeout");
            stronger_self->server->launchError(stronger_self->endpoint);
        });
    }
}

void udp_session::flush() {
    begin_session();
}
