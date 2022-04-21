/*
* @file_name: tcp_server.hpp
* @date: 2022/04/04
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

#ifndef TOOLKIT_TCP_SERVER_HPP
#define TOOLKIT_TCP_SERVER_HPP
#include "event_poller_pool.hpp"
#include "socket_helper.hpp"
#include "spdlog/logger.hpp"
#include "net/deprecated/tcp_session.hpp.tmp"
#include <Util/nocopyable.hpp>
#include <asio.hpp>
#include <mutex>
#include <vector>
class tcp_server;

class tcp_server : public std::enable_shared_from_this<tcp_server>, public noncopyable {
public:
    using endpoint_type = typename asio::ip::tcp::socket::endpoint_type;
public:
    tcp_server() = default;
public:
    template<typename session_type, typename...Args>
    void start(const endpoint_type& endpoint, Args&&...args) {
        std::weak_ptr<tcp_server> self(shared_from_this());
        event_poller_pool::Instance().get_poller(false)->template async([&, self]() {
            auto &poller = event_poller_pool::Instance().get_poller(false);
            auto acceptor = std::make_shared<asio::ip::tcp::acceptor>(poller->get_executor(), endpoint);
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            stronger_self->start_listen<session_type>(acceptor, std::forward<Args>(args)...);
        });
    }
private:
    template<typename session_type, typename...Args>
    void start_listen(const std::shared_ptr<asio::ip::tcp::acceptor> &acceptor, Args&&...args) {
        auto &poller = event_poller_pool::Instance().get_poller(false);
        auto _socket_ = std::make_shared<asio::ip::tcp::socket>(poller->get_executor());
        std::weak_ptr<tcp_server> self(shared_from_this());
        auto async_func = [&, _socket_, poller, self, acceptor](const std::error_code &e) {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                Warn("the tcp server has shutdown");
                return;
            }
            if (e) {
                Error("accept error, {}", e.message());
            }
            else{
                std::shared_ptr<session_type> session_(new session_type(*poller, *_socket_, std::forward<Args>(args)...));
                poller->template async([stronger_self, acceptor, session_]() {
                    session_->onConnected();
                    session_->begin_read();
                });
            }
            stronger_self->template start_listen<session_type>(acceptor, std::forward<Args>(args)...);
        };
        acceptor->template async_accept(*_socket_, async_func);
    }
};

#endif//TOOLKIT_TCP_SERVER_HPP
