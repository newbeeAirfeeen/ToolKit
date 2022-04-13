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
#include "session_helper.hpp"
#include "spdlog/logger.hpp"
#include "tcp_session.hpp"
#include <Util/nocopyable.hpp>
#include <asio.hpp>
#include <mutex>
#include <vector>
class tcp_server;

class tcp_server : public std::enable_shared_from_this<tcp_server>, public noncopyable {
public:
    using acceptor_pointer = std::shared_ptr<asio::ip::tcp::acceptor>;

public:
    tcp_server() = default;
    ~tcp_server();

public:
#ifdef SSL_ENABLE
    template<typename session_type>
    void start(unsigned short port, const std::string &address = "0.0.0.0", bool ipv4 = true,
               const std::shared_ptr<context>& _context = nullptr) {
#else
    template<typename session_type>
    void start(unsigned short port, const std::string &address = "0.0.0.0", bool ipv4 = true) {
#endif
        std::weak_ptr<tcp_server> self(shared_from_this());
        event_poller_pool::Instance().get_poller(false)->template async([=]() {
            using acceptor_func_type = std::function<void(const std::error_code &)>;
            auto &poller = event_poller_pool::Instance().get_poller(false);
            auto acceptor = std::make_shared<asio::ip::tcp::acceptor>(poller->get_executor(), asio::ip::tcp::endpoint(ipv4 ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), port));
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
#ifdef SSL_ENABLE
            stronger_self->start_listen<session_type>(acceptor, _context);
#else
            stronger_self->start_listen<session_type>(acceptor);
#endif
        });
    }

    /*!
     * 停止当前tcp_server，并清除跟此server相关的所有会话
     */
    void stop();

    /*!
     * 得到tcp连接的会话总数。
     * @return 当前在线人数
     */
    size_t get_tcp_session_count() const;
private:
#ifdef SSL_ENABLE
    template<typename session_type>
    void start_listen(const std::shared_ptr<asio::ip::tcp::acceptor> &acceptor,
                      const std::shared_ptr<context>& _context = nullptr) {
#else
    template<typename session_type>
    void start_listen(const std::shared_ptr<asio::ip::tcp::acceptor> &acceptor) {
#endif
        auto &poller = event_poller_pool::Instance().get_poller(false);
        auto _socket_ = std::make_shared<asio::ip::tcp::socket>(poller->get_executor());
        std::weak_ptr<tcp_server> self(shared_from_this());
#ifdef SSL_ENABLE
        auto async_func = [_socket_, acceptor, poller, self, _context](const std::error_code &e) {
#else
        auto async_func = [_socket_, acceptor, poller, self](const std::error_code &e) {
#endif
            auto stronger_self = self.lock();
            if (!stronger_self) {
                Warn("the tcp server has shutdown");
                return;
            }
            if (e) {
                Error("accept error, {}", e.message());
            } else {
#ifdef SSL_ENABLE
                std::shared_ptr<session_type> session_(new session_type(*_socket_, *poller, _context));
#else
                std::shared_ptr<session_type> session_(new session_type(*_socket_, *poller));
#endif
                //切换到自己的线程处理
                poller->template async([stronger_self, acceptor, session_]() {
                    auto create_ret = get_session_helper().create_session(session_);
                    if (!create_ret) {
                        Error("create tcp_session error");
                    } else {
                        Trace("目前在线人数: {}", stronger_self->get_tcp_session_count());
                        session_->begin_session();
                        std::lock_guard<std::mutex> lmtx(stronger_self->mtx);
                        stronger_self->acceptors.push_back(acceptor);
                    }
                });
            }
#ifdef SSL_ENABLE
            stronger_self->template start_listen<session_type>(acceptor, _context);
#endif
        };
        acceptor->template async_accept(*_socket_, async_func);
    }

private:
    std::mutex mtx;
    std::vector<acceptor_pointer> acceptors;
};

#endif//TOOLKIT_TCP_SERVER_HPP
