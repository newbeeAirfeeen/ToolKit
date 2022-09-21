/*
* @file_name: srt_session_base.hpp
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

#ifndef TOOLKIT_SRT_SESSION_BASE_HPP
#define TOOLKIT_SRT_SESSION_BASE_HPP
#include "executor.hpp"
#include "srt_socket_service.hpp"
#include <memory>

namespace srt {
    class srt_server;
    class srt_session_base : public srt_socket_service {
        friend class srt_server;

    public:
        srt_session_base(const std::shared_ptr<asio::ip::udp::socket> &_sock, const event_poller::Ptr &context);
        ~srt_session_base() override = default;

    protected:
        void onRecv(const std::shared_ptr<buffer> &) override = 0;
        virtual void onConnected() = 0;
        virtual void onError(const std::error_code &e) = 0;

    protected:
        ///// override from srt_socket_service
        const asio::ip::udp::endpoint &get_remote_endpoint() final;
        const asio::ip::udp::endpoint &get_local_endpoint() final;
        std::shared_ptr<executor> get_executor() const final;
        /// 统一数据接收接口
    private:
        void on_session_timeout();
        void set_parent(const std::shared_ptr<srt_server> &);
        void set_current_remote_endpoint(const asio::ip::udp::endpoint &);
        void set_cookie(uint32_t);
        uint32_t get_cookie() final;
        void begin_session();
        void receive(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &buff);
        void on_connected() final;
        void send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) final;
        void on_error(const std::error_code &e) final;

    private:
        std::weak_ptr<srt_server> _parent_server;
        asio::ip::udp::socket &_sock;
        asio::ip::udp::endpoint _remote;
        asio::ip::udp::endpoint _local;
        uint32_t cookie_ = 0;
        std::shared_ptr<executor> executor_;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SESSION_BASE_HPP
