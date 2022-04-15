/*
* @file_name: udp_client.cpp
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
#include "udp_client.hpp"
#include "event_poller_pool.hpp"
#include "socket_base.hpp"
#include "udp_server.hpp"
#include <thread>

#ifdef SSL_ENABLE
udp_client::udp_client(event_poller &poller, const std::shared_ptr<context> &_context)
    : poller(poller), sock(poller.get_executor()), udp_helper(sock), socket_sender<asio::ip::udp::socket, udp_client>(sock, poller) { init(); }
#else
udp_client::udp_client(event_poller &poller) : udp_session(poller, {}, false, sock), sock(poller.get_executor()), socket_sender<asio::ip::udp::socket, udp_client>(sock, poller) { init(); }
{
}
#endif

std::shared_ptr<udp_client> udp_client::shared_from_this_subtype(){
    return std::static_pointer_cast<udp_client>(shared_from_this());
}

void udp_client::connect(const endpoint_type &endpoint) {
    std::lock_guard<decltype(mtx)> lmtx(mtx);
    socket_sender<asio::ip::udp::socket, udp_client>::clear();
    sock.connect(endpoint);
    return read_l();
}

void udp_client::setOnRecv(const std::function<void(const char *, size_t, const endpoint_type &)> &func) {
    if (func)
        this->on_recv_func = func;
}

void udp_client::setOnError(const std::function<void(const std::error_code &)> &func) {
    if (func)
        this->on_error_func = func;
}
void udp_client::init() {
    this->on_recv_func = [this](const char *data, size_t length, const endpoint_type &endpoint) {
        Info("recv from {}:{} {} bytes, {}", endpoint.address().to_string(), endpoint.port(), length, data);
        if (this->sock.remote_endpoint() != endpoint) {
            Warn("the sock peer endpoint {} , which is different from current endpoint {}", this->sock.remote_endpoint().address().to_string(), endpoint.address().to_string());
        }
    };
    this->on_error_func = [this](const std::error_code &e) {
        Error("client happens error: {}", e.message());
    };
}
void udp_client::read_l() {
    auto stronger_self(shared_from_this());
    auto remote_point = std::make_shared<endpoint_type>();
    auto result = [stronger_self, remote_point](const std::error_code &e, size_t length) {
        if (e) {
            stronger_self->on_error_func(e);
        }
        if (length)
            stronger_self->on_recv_func(stronger_self->buff_, length, *remote_point);
        return stronger_self->read_l();
    };

    stronger_self->sock.async_receive_from(asio::buffer(stronger_self->buff_, sizeof(stronger_self->buff_)), *remote_point, result);
}
