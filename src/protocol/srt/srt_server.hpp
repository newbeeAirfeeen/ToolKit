/*
* @file_name: srt_server.hpp
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

#ifndef TOOLKIT_SRT_SERVER_HPP
#define TOOLKIT_SRT_SERVER_HPP
#include "asio.hpp"
#include "srt_session_base.hpp"
#include <mutex>
#include <unordered_map>
#include <vector>
namespace srt {

    class srt_server : public std::enable_shared_from_this<srt_server> {
    public:
        using on_create_session_func = std::function<std::shared_ptr<srt_session_base>(const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context)>;

    public:
        srt_server();
        ~srt_server() = default;

    public:
        void start(const asio::ip::udp::endpoint &endpoint);
        void remove_cookie_session(uint32_t);
        void remove_session(uint32_t);
        void add_connected_session(const std::shared_ptr<srt_session_base> &session);
        /// 创建会话的回调
        void on_create_session(const on_create_session_func &f);

    private:
        void on_receive(const std::shared_ptr<buffer> &, const asio::ip::udp::endpoint &, const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &);
        void handle_handshake(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &, const asio::ip::udp::endpoint &, const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &);
        void handle_data(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &);

    private:
        std::shared_ptr<asio::ip::udp::socket> create(const event_poller::Ptr &, const asio::ip::udp::endpoint &);
        void start(const std::shared_ptr<asio::ip::udp::socket> &, const event_poller::Ptr &context);
        void read_l(const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context);
        std::shared_ptr<srt_session_base> get_session(uint32_t);
        std::shared_ptr<srt_session_base> get_session_with_cookie(uint32_t);

    private:
        std::vector<std::shared_ptr<asio::ip::udp::socket>> _socks;
        std::recursive_mutex _cookie_mtx;
        std::unordered_map<uint32_t, std::shared_ptr<srt_session_base>> _handshake_map;
        /// 会话管理map
        /// socket id
        std::recursive_mutex mtx;
        std::unordered_map<uint32_t, std::shared_ptr<srt_session_base>> _session_map_;
        on_create_session_func _on_create_session_func_;
    };
}// namespace srt
#endif//TOOLKIT_SRT_SERVER_HPP
