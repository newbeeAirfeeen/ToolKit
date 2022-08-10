/*
* @file_name: srt_socket.hpp
* @date: 2022/08/08
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

#ifndef TOOLKIT_SRT_SOCKET_SERVICE_HPP
#define TOOLKIT_SRT_SOCKET_SERVICE_HPP
#include "deadline_timer.hpp"
#include "net/asio.hpp"
#include "net/buffer.hpp"
#include "srt_socket_base.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
///#include "socket_statistic.hpp"
namespace srt {

    class srt_socket_service : public srt_socket_base, public std::enable_shared_from_this<srt_socket_service> {
    public:
        using clock_type = typename std::chrono::steady_clock;
        using time_point = typename std::chrono::steady_clock::time_point;

    public:
        explicit srt_socket_service(asio::io_context &executor);
        virtual ~srt_socket_service() = default;

    public:
        srt_socket_service(const srt_socket_service &) = delete;
        srt_socket_service &operator=(const srt_socket_service &) = delete;
        virtual void begin();
    protected:
        void connect();
        void input_packet(const std::shared_ptr<buffer> &buff);
        bool is_connected() override final;
        //// 需要在连接的时候调用
        virtual const asio::ip::udp::endpoint &get_remote_endpoint() = 0;
        virtual const asio::ip::udp::endpoint &get_local_endpoint() = 0;
        /// 连接成功
        virtual void on_connected() = 0;
        /// 发送行为
        virtual void send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) = 0;
        /// 出错调用
        virtual void on_error(const std::error_code &e) = 0;

    private:
        void send_reject();
        /// 数据统一出口
        void send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where);
        /// 发送keepalive数据
        void do_keepalive();

    private:
        /// 用于客户端握手
        void handle_server_induction(const std::shared_ptr<buffer> &buff);
        void handle_server_conclusion(const std::shared_ptr<buffer> &buff);
        void handle_data_and_control(const std::shared_ptr<buffer> &buff);

    private:
        void on_common_timer_expired(const int &);
        /// 保活定时器
        void on_keep_alive_expired(const int &);

    private:
        asio::io_context &poller;
        /// 通常的定时器,处理ack,nak
        std::shared_ptr<deadline_timer<int>> common_timer;
        std::shared_ptr<deadline_timer<int>> keep_alive_timer;
        mutable_basic_buffer<char> receive_cache;
        /// 握手缓存
        std::shared_ptr<buffer> handshake_buffer;
        /// keep alive 缓存
        std::shared_ptr<buffer> keep_alive_buffer;
        /// 握手上下文
        std::function<void(const std::shared_ptr<buffer> &)> _next_func;
        /// 第一次尝试连接的时间
        time_point first_connect_point;
        /// 上一次发送数据的时间
        time_point last_send_point;
        /// 上一次接收数据的时间
        time_point last_receive_point;
        /// 是否已经建立连接
        std::atomic<bool> _is_connected{false};
        /// 上一次发送包的序号
        uint32_t sequence_number = 0;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SOCKET_SERVICE_HPP
