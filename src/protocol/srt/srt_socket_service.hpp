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
#include <tuple>
#include "deadline_timer.hpp"
#include "net/asio.hpp"
#include "net/buffer.hpp"
#include "packet_interface.hpp"
#include "srt_ack.hpp"
#include "srt_handshake.h"
#include "srt_packet.h"
#include "srt_socket_base.hpp"
namespace srt {
    using buffer_pointer = std::shared_ptr<buffer>;
    class srt_socket_service : public srt_socket_base, public std::enable_shared_from_this<srt_socket_service> {
    public:
        using time_point = typename std::chrono::steady_clock::time_point;
        using packet_pointer = typename packet_interface<std::shared_ptr<buffer>>::packet_pointer;
        using ack_entry = std::tuple<uint32_t, time_point>; /// ack number + counts
    public:
        explicit srt_socket_service(asio::io_context &executor);
        ~srt_socket_service() override = default;
        srt_socket_service(const srt_socket_service &) = delete;
        srt_socket_service &operator=(const srt_socket_service &) = delete;

    public:
        asio::io_context &get_poller();
        //// 发送数据
        int async_send(const char *, size_t length);
        int async_send(const std::shared_ptr<buffer> &);

    protected:
        void connect();
        void connect_as_server();
        void input_packet(const std::shared_ptr<buffer> &buff);
        void input_packet(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &buff);
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
        virtual void onRecv(const std::shared_ptr<buffer> &) = 0;
        /// 出错调用
        virtual void on_error(const std::error_code &e) = 0;
        virtual uint32_t get_cookie();

    protected:
        void on_error_in(const std::error_code &e);

    private:
        //// send_queue
        void on_sender_packet(const packet_pointer &type);
        void on_sender_drop_packet(size_t begin, size_t end);
        /// receive queue
        void on_receive_packet(const packet_pointer &type);
        void on_receive_drop_packet(size_t begin, size_t end);

    private:
        void send_reject(int e, const std::shared_ptr<buffer> &buf);
        /// 数据统一出口
        void send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where);
        void on_connect_in();
        void do_keepalive();
        void do_nak();
        void do_nak_in();
        /// Round-trip time (RTT) in SRT is estimated during the transmission of data packets based on
        /// difference in time between an ACK packet is send out and a corresponding ACK_ACK is received
        /// back by SRT receiver.
        /// An ACK send by the receiver triggers an ACK_ACK from the sender with minimal processing delay.
        void do_ack();
        void do_ack_in();
        void do_ack_ack(uint32_t ack_number);
        void do_drop_request(size_t begin, size_t end);
        void do_shutdown();

        template<typename _duration>
        inline uint32_t get_time_from(const std::chrono::steady_clock::time_point &last_time_point) {
            return static_cast<uint32_t>(std::chrono::duration_cast<_duration>(std::chrono::steady_clock::now() - last_time_point).count());
        }

    private:
        void handle_reject(int e);
        /// 用于客户端握手
        void handle_server_induction(const std::shared_ptr<buffer> &buff);
        void handle_server_induction_1(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff);
        void handle_server_conclusion(const std::shared_ptr<buffer> &buff);
        void handle_server_conclusion_1(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff);
        /// 用于服务端握手
        void handle_client_induction(const std::shared_ptr<buffer> &buff);
        void handle_client_induction_1(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff);
        void handle_client_conclusion(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<handshake_context> &context, const std::shared_ptr<buffer> &buff);

        void handle_receive(const std::shared_ptr<buffer> &buff);
        void handle_receive_1(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &buff);

        void handle_control(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &);
        void handle_data(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &);
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

    private:
        asio::io_context &poller;
        /// 通常的定时器,处理ack,nak
        std::shared_ptr<deadline_timer<int>> common_timer;
        std::shared_ptr<deadline_timer<int>> keep_alive_timer;
        /// 握手缓存
        std::shared_ptr<buffer> handshake_buffer;
        /// keep alive 缓存
        std::shared_ptr<buffer> keep_alive_buffer;
        /// 握手上下文
        std::shared_ptr<handshake_context> _handshake_context;
        int handshake_conclusion = 0;
        bool report_nak_begin = false;
        bool ack_begin = false;
        bool occur_error = false;
        uint32_t ack_number = 1;
        std::function<void(const std::shared_ptr<buffer> &)> _next_func;
        std::function<void(const std::shared_ptr<srt_packet> &, const std::shared_ptr<buffer> &)> _next_func_with_pkt;
        /// 第一次尝试连接的时间
        time_point connect_point;
        /// 上一次发送数据的时间
        time_point last_send_point;
        /// 上一次接收数据的时间
        time_point last_receive_point;
        /// 是否已经建立连接
        std::atomic<bool> _is_open{false};
        std::atomic<bool> _is_connected{false};
        /// 发送队列
        std::shared_ptr<packet_send_interface<std::shared_ptr<buffer>>> _sender_queue;
        /// 接收队列
        std::shared_ptr<packet_receive_interface<std::shared_ptr<buffer>>> _receive_queue;

        std::shared_ptr<packet_calculate_window<16, 64>> _packet_receive_rate_;
        //// ack
        ack_entry _ack_entry;
        std::shared_ptr<srt_ack_queue> _ack_queue_;
        /// 上一次接收ack的时间
        time_point last_ack_response;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SOCKET_SERVICE_HPP
