/*
* @file_name: tcp_session.hpp
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
#include "tcp_session.hpp"
#include "session_helper.hpp"
#include "spdlog/logger.hpp"
#include <chrono>

#ifdef SSL_ENABLE
tcp_session::tcp_session(typename asio::ip::tcp::socket &socket_, event_poller &poller,
                         const std::shared_ptr<context> &context) : poller(poller), sock(std::move(socket_)),
                                                                    _context(context), recv_timer(poller.get_executor()),
                                                                    socket_sender<asio::ip::tcp::socket, tcp_session>(std::ref(sock), poller){
                                                                            set_no_delay();
}
tcp_session::tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> &pair_,
                         const std::shared_ptr<context> &_context_)
    : _context(_context_), recv_timer(pair_.first->get_executor()),
      sock(std::move(*pair_.second)),
      poller(*pair_.first),
      socket_sender<asio::ip::tcp::socket, tcp_session>(std::ref(sock), *pair_.first) {
    _is_server = false;
    set_no_delay();
}
#else
tcp_session::tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> &pair_)
    : recv_timer(pair_.first->get_executor()),
      poller(*pair_.first),
      sock(std::move(*pair_.second)),
      socket_sender<asio::ip::tcp::socket, tcp_session>(std::ref(sock), poller){
}
tcp_session::tcp_session(socket_type &socket_, event_poller &poller)
    : poller(poller), sock(std::move(socket_)), recv_timer(poller.get_executor()),
      socket_sender<asio::ip::tcp::socket, tcp_session>(std::ref(sock), poller){}
#endif
tcp_session::~tcp_session() {
    Trace("~session");
    sock.close();
}

event_poller &tcp_session::get_poller() {
    return this->poller;
}

void tcp_session::onRecv(const char *data, size_t length) {
    Info("recv data {} length", length);
    send("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nyes", 67);
}

void tcp_session::onError(const std::error_code &e) {
    Error(e.message());
}

void tcp_session::before_begin_session() {
}

void tcp_session::set_send_time_out(size_t time) {
    send_time_out.store(time);
    return this->reset_timer(time);
}

void tcp_session::set_recv_time_out(size_t time) {
    recv_time_out.store(time);
    this->recv_timer.expires_after(std::chrono::seconds(time));
}

void tcp_session::set_recv_buffer_size(size_t size) {
    asio::socket_base::receive_buffer_size recv_buf_size(size);
    sock.set_option(recv_buf_size);
}

void tcp_session::set_send_buffer_size(size_t size) {
    asio::socket_base::send_buffer_size send_buf_size(size);
    sock.set_option(send_buf_size);
}

void tcp_session::set_send_low_water_mark(size_t size) {
    asio::socket_base::send_low_watermark slw(size);
    sock.set_option(slw);
}

void tcp_session::set_recv_low_water_mark(size_t size) {
    asio::socket_base::receive_low_watermark rlw(size);
    sock.set_option(rlw);
}

void tcp_session::set_no_delay(bool no_delay) {
    asio::ip::tcp::no_delay _no_delay(no_delay);
    sock.set_option(_no_delay);
}

void tcp_session::send(basic_buffer<char> &buf) {
    return socket_sender<socket_type, tcp_session>::send(std::ref(buf));
}

void tcp_session::send(const char *data, size_t length) {
    basic_buffer<char> tmp(data, length);
    return send(std::ref(tmp));
}

void tcp_session::send(std::string &str) {
    basic_buffer<char> tmp_buf(std::move(str));
    return send(std::ref(tmp_buf));
}

bool tcp_session::is_server() const {
    return _is_server;
}

using endpoint_type = typename tcp_session::endpoint_type;
endpoint_type tcp_session::get_local_endpoint() {
    return sock.local_endpoint();
}

endpoint_type tcp_session::get_remote_endpoint() {
    return sock.remote_endpoint();
}


void tcp_session::begin_session() {
    Trace("begin tcp session");
    before_begin_session();
    return this->read_l();
}

void tcp_session::read_l() {
    auto stronger_self = std::static_pointer_cast<tcp_session>(shared_from_this());
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
            stronger_self->sock.shutdown(asio::socket_base::shutdown_receive);
        });
    }
    auto read_function = [stronger_self](const std::error_code &e, size_t length) {
        if (e) {
            stronger_self->onError(e);
            stronger_self->recv_timer.cancel();
            if (stronger_self->_is_server)
                get_session_helper().remove_session(stronger_self);
            return;
        }
        stronger_self->onRecv(stronger_self->buffer, length);
        stronger_self->read_l();
    };
    std::shared_ptr<basic_session> session_ptr(shared_from_this());
    sock.async_read_some(asio::buffer(stronger_self->buffer, 10240), read_function);
}

size_t tcp_session::get_send_time_out(){
    return this->send_time_out.load(std::memory_order_relaxed);
}
using socket_type = typename tcp_session::socket_type;
socket_type &tcp_session::get_sock() {
    return sock;
}