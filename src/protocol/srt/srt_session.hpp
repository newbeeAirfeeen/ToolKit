/*
* @file_name: srt_session.hpp
* @date: 2022/08/20
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef TOOLKIT_SRT_SESSION_HPP
#define TOOLKIT_SRT_SESSION_HPP
#include "net/asio.hpp"
#include "net/buffer.hpp"
#include "spdlog/logger.hpp"
#include <memory>

namespace srt {
    class srt_server;
    class srt_session : public std::enable_shared_from_this<srt_session> {
    public:
        srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, asio::io_context &context);
        virtual ~srt_session();
    public:
        void set_parent(const std::shared_ptr<srt_server> &);
        void set_current_remote_endpoint(const asio::ip::udp::endpoint &);
        asio::io_context &get_poller();
        void set_max_receive_time_out(uint32_t ms);
    public:
        virtual void begin_session();
        virtual void receive(const std::shared_ptr<buffer> &buff);
    private:
        void on_session_timeout();
    private:
        asio::io_context &context;
        /// 握手前的定时器。防止发一个包后不发数据，导致srt_server一直存有引用
        asio::steady_timer timer;
        std::chrono::steady_clock::time_point last_receive_time_point = std::chrono::steady_clock::now();
        std::weak_ptr<srt_server> _parent_server;
        asio::ip::udp::socket &_sock;
        uint32_t time_out = 10;
        asio::ip::udp::endpoint _remote;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SESSION_HPP
