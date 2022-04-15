/*
* @file_name: udp_server.cpp
* @date: 2022/04/09
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
#include "udp_server.hpp"
#include "event_poller_pool.hpp"
#include "spdlog/logger.hpp"
#include "udp_session.hpp"
udp_server::udp_server(event_poller &poller) : poller(poller), asio::ip::udp::socket(poller.get_executor()) {
    buff.resize(2048);
}

void udp_server::stop() {
}

void udp_server::read_datagram() {
    auto stronger_self(shared_from_this());
    std::shared_ptr<endpoint_type> endpoint = std::make_shared<endpoint_type>();
    auto recv_func = [stronger_self, endpoint](const std::error_code &er, size_t length) {
        if (er) {
            Error(er.message());
        }
        if (length > 0) {
            stronger_self->buff.resize(length);
            stronger_self->transmit_message(stronger_self->buff, *endpoint);
            stronger_self->buff.resize(2048);
        }
        return stronger_self->read_datagram();
    };
    async_receive_from(asio::buffer(const_cast<char *>(stronger_self->buff.data()), stronger_self->buff.size()), *endpoint, recv_func);
}

void udp_server::transmit_message(basic_buffer<char> &buf, const endpoint_type &endpoint) {
    auto it = session_map.find(endpoint);
    if (it == session_map.end()) {
        auto session = std::make_shared<udp_session>(*event_poller_pool::Instance().get_poller(false), _context, static_cast<asio::ip::udp::socket&>(*this));
        session_map.emplace(endpoint, session);
        //开始会话
        session->attach_server(this);
        session->begin_session();
        session->launchRecv(buf, endpoint);
        return;
    }
    //刷新定时器
    it->second->flush();
    it->second->launchRecv(buf, endpoint);
}

void udp_server::launchError(const endpoint_type &endpoint) {
    //切换到自己的线程
    std::weak_ptr<udp_server> self(std::static_pointer_cast<udp_server>(shared_from_this()));
    poller.async([endpoint, self]() {
        auto stronger_self = self.lock();
        if (!stronger_self) {
            return;
        }
        stronger_self->remove_session(endpoint);
    });
}

void udp_server::remove_session(const endpoint_type &endpoint) {
    auto begin = session_map.begin();
    session_map.erase(endpoint);
}