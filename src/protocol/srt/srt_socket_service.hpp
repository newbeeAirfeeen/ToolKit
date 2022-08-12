/*
* @file_name: srt_socket.hpp
* @date: 2022/08/08
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

#ifndef TOOLKIT_SRT_SOCKET_SERVICE_HPP
#define TOOLKIT_SRT_SOCKET_SERVICE_HPP
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

#include "deadline_timer.hpp"
#include "net/asio.hpp"
#include "net/buffer.hpp"
#include "sender_queue.hpp"
#include "socket_statistic.hpp"
#include "srt_handshake.h"
#include "srt_packet.h"
#include "srt_socket_base.hpp"
namespace srt {
    using buffer_pointer = std::shared_ptr<buffer>;
    class srt_socket_service : public srt_socket_base, public sender_queue<buffer_pointer>, public std::enable_shared_from_this<srt_socket_service> {
    public:
        using clock_type = typename std::chrono::steady_clock;
        using time_point = typename std::chrono::steady_clock::time_point;

    public:
        explicit srt_socket_service(asio::io_context &executor);
        ~srt_socket_service() override = default;
        srt_socket_service(const srt_socket_service &) = delete;
        srt_socket_service &operator=(const srt_socket_service &) = delete;

    protected:
        void connect();
        void input_packet(const std::shared_ptr<buffer> &buff);
        bool is_open() final;
        bool is_connected() final;
        virtual void begin();
        //// 需要在连接的时候调用
        virtual const asio::ip::udp::endpoint &get_remote_endpoint() = 0;
        virtual const asio::ip::udp::endpoint &get_local_endpoint() = 0;
        /// 连接成功
        virtual void on_connected() = 0;
        /// 发送行为
        virtual void send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) = 0;
        /// 出错调用
        virtual void on_error(const std::error_code &e) = 0;
    protected:
        void async_send(const std::shared_ptr<buffer> &);
    protected:
        //// send_queue
        typename sender_queue<buffer_pointer>::pointer get_shared_from_this() override;
        void send(const block_type &type) override;
        void on_drop_packet(size_type type) override;
    private:
        void send_reject(int e, const std::shared_ptr<buffer> &buf);
        /// 数据统一出口
        void send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where);
        void on_connect_in();
        void on_error_in(const std::error_code &e);
        void do_keepalive();
        void do_nak();
        /// Round-trip time (RTT) in SRT is estimated during the transmission of data packets based on
        /// difference in time between an ACK packet is send out and a corresponding ACK_ACK is received
        /// back by SRT receiver.
        /// An ACK send by the receiver triggers an ACK_ACK from the sender with minimal processing delay.
        void do_ack();
        void do_ack_ack();
        void do_drop_request();
        void do_shutdown();
        inline uint32_t get_time_from_begin();

    private:
        /// 用于客户端握手
        void handle_reject(int e);
        void handle_server_induction(const std::shared_ptr<buffer> &buff);
        void handle_server_conclusion(const std::shared_ptr<buffer> &buff);
        void handle_receive(const std::shared_ptr<buffer> &buff);
        void handle_control(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_data(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_keep_alive(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_nak(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_ack_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_peer_error(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_drop_request(const srt_packet &pkt, const std::shared_ptr<buffer> &);
        void handle_shutdown(const srt_packet &pkt, const std::shared_ptr<buffer> &);

    private:
        /// 定时器回调
        void on_common_timer_expired(const int &);
        /// 保活定时器
        void on_keep_alive_expired(const int &);

        inline uint32_t get_next_packet_sequence_number();
        inline uint32_t get_next_packet_message_number();
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
        /// std::shared_ptr<handshake_context> _handshake_context;
        std::function<void(const std::shared_ptr<buffer> &)> _next_func;
        /// 第一次尝试连接的时间
        time_point first_connect_point;
        /// 上一次发送数据的时间
        time_point last_send_point;
        /// 上一次接收数据的时间
        time_point last_receive_point;
        /// 是否已经建立连接
        std::atomic<bool> _is_open{false};
        std::atomic<bool> _is_connected{false};
        /// 上一次发送包的序号
        uint32_t packet_sequence_number = 0;
        uint32_t message_number = 0;
        /// 用于统计包发送数据
        std::shared_ptr<socket_statistic> _sock_send_statistic;
        /// 用于统计接收的包
        std::shared_ptr<socket_statistic> _sock_receive_statistic;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SOCKET_SERVICE_HPP
