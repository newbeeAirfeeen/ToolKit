/*
* @file_name: srt_core.hpp
* @date: 2022/04/25
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
#ifndef TOOLKIT_SRT_CORE_HPP
#define TOOLKIT_SRT_CORE_HPP
#include "net/socket_helper.hpp"
#include "srt_control_type.h"
#include "srt_handshake.h"
#include "srt_packet.h"
#include "srt_status.hpp"
#include <Util/nocopyable.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <net/asio.hpp>
#include <net/buffer.hpp>
#include <net/event_poller.hpp>
#include <set>

namespace srt{
    //// for socket id generator and destroy.
    class socket_id{
    public:
        socket_id();
        ~socket_id();
        bool reset(uint32_t);
        uint32_t val() const;
    private:
        uint32_t value = 0;
    };
    /**
     * when in this implementation, srt_core is single thread mode, buf use one loop with multiplexing.
     * avoid thread switch. in other hand, guarantee the thread safe.
     * for this version, srt only support caller-listener mode
     */
    class srt_core : public noncopyable, public std::enable_shared_from_this<srt_core>{
    public:
        using socket_type = asio::ip::udp::socket;
        using steady_timer = asio::steady_timer;
        using endpoint_type = typename asio::ip::udp::socket::endpoint_type;
    public:
        srt_core(event_poller& poller, asio::ip::udp::socket& sock, bool is_server = true);
        /**
         * @description bind local endpoint
         * @param endpoint
         */
        void bind_local(const endpoint_type& endpoint);
        /**
         * set connect_time out ,
         * @param miliseconds
         */
        void set_connect_time_out(size_t miliseconds);
        /**
         * set max mtu for single packet
         * @param mtu_size
         */
        void set_mtu(uint32_t mtu_size);
        /**
         * set max window flow size
         * @param window_size
         */
        void set_window_size(uint32_t window_size);
        /**
         * @description register internal srt core error for socket error so that could handle socket error
         * @param err_func error callback
         */
        void register_error_function(const std::function<void(const std::error_code&)>& err_func);
        /**
         * @description connect endpoint asynchronous. the callback is result of connection
         * @param endpoint the peer endpoint.
         * @param connect_func connections result callback.
         */
        void async_connect(const endpoint_type& endpoint, const std::function<void(const std::error_code&)>& connect_func);
        void async_receive(buffer& buf, const std::function<void(size_t, const std::error_code&)>& receive_func);
        void async_send(buffer& buf, const std::function<void(size_t, const std::error_code&)>& send_func);
        void async_shutdown(const std::function<void(const std::error_code&)>& shutdown_func);
        void close();
    private:
        /**
         * the internal handle execution srt core loop @n
         * this function is not thread safe.
         */
        void loop();
        void loop_l();
        /**
         * @description: the internal read function
         */
        void read_internal();
        /**
         * @description: the internal write function
         */
        void write_internal();
        /**
         * connect internal callback for asio
         * @param connect_func
         */
        void async_connect_l(const std::error_code& e, const std::function<void(const std::error_code&)>& connect_func);
    private:
        void start_loop_timer();
        void handle_receive_queue_packets();
        void handle_receive_control_packet(const std::shared_ptr<srt_packet>& control_packet);
        void handle_handshake(const control_packet_context& ctx, const std::shared_ptr<srt_packet>& handshake_packet);
        /**
         * for client use, induction response
         */
        void handle_handshake_induction_client_1(const control_packet_context& ctx, const srt_packet& handshake_packet);
        /**
         * for client use, conduction response
         * @param ctx
         * @param handshake_packet
         */
        void handle_handshake_conduction_client_2(const control_packet_context& ctx, const srt_packet& handshake_packet);
        /**
         * for server use,
         * @param ctx
         * @param handshake_packet
         */
        void handle_handshake_induction_server_1(const control_packet_context& ctx, const srt_packet& handshake_packet);
        void handle_keep_alive(const std::shared_ptr<srt_packet>& keep_alive_packet);
        void handle_ack(const std::shared_ptr<srt_packet>& ack_packet);
        void handle_nak(const std::shared_ptr<srt_packet>& nak_packet);
        void handle_congestion_warn(const std::shared_ptr<srt_packet>& congestion_packet);
        void handle_shutdown(const std::shared_ptr<srt_packet>& shutdown_packet);
        void handle_ack_ack(const std::shared_ptr<srt_packet>& ack_ack_packet);
        void handle_dro_req(const std::shared_ptr<srt_packet>& dro_req_packet);
        void handle_peer_error(const std::shared_ptr<srt_packet>& peer_error_packet);
        void handle_user_defined_type(const std::shared_ptr<srt_packet>& user_defined_packet);
        void handle_receive_data_packet(const std::shared_ptr<srt_packet>& data_packet);
        void send_connect_packet(const handshake_packet& pkt, bool init_ack_nak);
        void put_keep_alive_packet_into_send_queue();
    private:
        /**
         * we use simple bake different from official algorithm.
         */
        void bake(uint32_t& current_cookie);
    private:
        void do_write_op();
        void do_send_ack_op();
        void do_send_ack_op_l();
        void do_send_nak_op();
        void do_send_nak_op_l();
        void do_send_keep_alive_op();
        void do_send_keep_alive_op_l();
    private:
        void set_initial_ack(uint32_t seq);
        inline uint32_t make_pkt_timestamp();
        inline uint32_t generate_sequence_number();
    private:
        void send_reject();
        void send_srt_packet(const std::shared_ptr<srt_packet>& pkt, bool after_send_keep_alive = false);
    private:
        /**
         * for recv udp socket
         * @param buf buffer
         * @param endpoint endpoint
         */
        void onRecv(basic_buffer<char>& buf, const endpoint_type& endpoint);
    private:
        /**
         * status init,
         */
        srt_status _srt_status = srt_init;
        endpoint_type local_endpoint;
        /**
         * whether is first receive packet
         */
        bool is_first_receive = true;
        /**
         * current sequence number for srt packet
         */
        uint32_t current_seq_number;
        /**
         * the kernel sending queue
         */
        std::map<uint32_t, std::shared_ptr<srt_packet>> send_queue;
        /**
         * the kernel sending queue which is lost_queue
         */
        std::map<uint32_t, std::shared_ptr<srt_packet>> send_lost_queue;
        /**
         * the kernel receive queue
         */
        std::map<uint32_t, std::shared_ptr<srt_packet>> receive_queue;
        bool is_in_loop = false;
        /**
         * indicate srt is server or client,
         */
        bool is_server = true;
        /**
         * socket with srt is used for send packet.
         */
        asio::ip::udp::socket& sock;
        /**
         * the binder thread(the binder loop)
         */
        event_poller& poller;
        /**
         * 3.2.3 Keep-alive p22
         * Keep-alive control packets are sent after a certain timeout from the last time any packet
         * (Control or Data) was sent.
         * the default timeout for keep-alive packet to be sent is 1 seconds.
         * when no data be sent in 1s, send keep_alive to avoid the connection closed.
         */
        const size_t keep_alive_internal = 1000;
        steady_timer keep_alive_timer;
        /**
         * 3.2.4 ACK(Acknowledgment) p25
         *
         * the Light ACK and Small ACK packets are used in cases when the receiver should acknowledge
         * received data packets more than every 10ms
         */
        size_t ack_interval = 10;
        steady_timer ack_timer;
        /**
         * 4.8.2 packet retransmission p53
         *
         * the SRT receiver send NAK control packets to notify the sender about missing packets.
         * the nak packet sending can be triggered immediately after a gap in sequence numbers of data
         * packets is detected.
         * upon reception of the NAK packet, the SRT sender prioritizes retransmissions of lost packets over
         * the regular data packets  to be transmitted for the first time
         * SRT Periodic NAK reports are sent with a period of (RTT + 4 * RTTVar) / 2 (so called NAKInterval),
         * with a 20 milliseconds floor
         * the time base is
         * nak_interval = max((RTT  + 4 * RTTVar) / 2, 20)
         *              = max(150, 20)
         *              = 150ms
         */
        size_t nak_interval = 150;
        steady_timer nak_timer;
        /**
         * 4.8.2 packet retransmission p53
         *
         * the srt sender maintains a list of lost packets(lost list) that is built from NAK reports,
         * When scheduling packet transmission, it looks to see if a packet in the loss list has priority and
         * sends it if so.
         * in this case, we use uint32_t type which means sequence number, and use set to order the packet.
         * when receive new packet, check it whether in lost packets struct, if have, removed packets from struct
         */
        std::set<uint32_t> loss_packets;
        /**
         * 4.10 Round-Trip Time Estimation  P54
         *
         * Round-trip time(RTT) is SRT is estimated during the transmission of data packets based on difference
         * in time between an ack packet is send out and a corresponding ACKACK packet is received back by the
         * SRT receiver.
         * An ACK sent by the receiver triggers an ACKACK from the sender with minimal processing delay.
         *
         * The SRT receiver records the time when an ACK is sent out.
         * the ACK carries a unique sequence number(independent of the data packet sequence number)
         * the ACKACK also carries the same sequence number.
         */
        steady_timer::time_point last_ack_sent = steady_timer::time_point::min();
        steady_timer::time_point last_ack_sent_back = steady_timer::time_point::min();
        /**
         * for connect timeout for handshake
         * 5000000 = 5s
         */
        size_t connect_time = 5000000;
        steady_timer connect_timer;
        /**
         * whether received induction_1.
         */
        bool receive_connect_induction_1 = false;
        /**
         * last ack send
         */
        uint32_t last_ack_send_seq = 0;
        /**
         * last fully ack received
         */
        uint32_t last_full_ack_recv_seq = 0;
        /**
         * The largest sequence number that HAS BEEN SENT
         */
        uint32_t ack_current_send_seq = 0;
        /**
         * next ack seq send to peer
         */
        uint32_t next_ack_send_seq = 0;
        /**
         * last ack seq sent from peer
         */
        uint32_t last_ack_send_back = 0;
        /**
         * 3.2.3. Keep-Alive
         * Keep-alive control packets are sent after a certain timeout from the
         * last time any packet (Control or Data) was sent.
         *
         * the variable save last packet was sent time point
         */
        typename steady_timer::time_point last_any_packet_sent = steady_timer::time_point::min();
        typename steady_timer::time_point last_any_packet_recv = steady_timer::time_point::min();
        typename steady_timer::time_point socket_start_time = steady_timer::time_point::min();
        /**
         * 4.10 Round-Trip Time Estimation  P54
         *
         * Upon receiving the ACKACK, SRT calculates the RTT by comparing the difference between the ACKACK arrival
         * time and the ACK departure time.
         * RTT is the current value that the receiver maintains and rtt is recent value that just calculated
         * from an ACK/ACKACK pair
         *
         * RTT = 7/8 * RTT + 1/8 * rtt.
         * RTT is ms time-based.
         *
         * RTTVar = 3/4 * RTTVar + 1/4 * abs(RTT - rtt)
         *
         * Both RTT and RTTVar are measured in microseconds
         * The initial value of RTT is 100 milliseconds. RTTVar is 50 millisecond.
         *
         * p55
         * receiver as well as the RTT variance(RTTVar) are sent with the next full acknowledgement
         * packet. the SRT sender updates its own RTT and RTTVar values using the same formulas as above
         */
        double RTT = 100;
        double rtt = 0;
        double RTTVar = 50;
        // for client to handshake
        socket_id _sock_id;
        /**
         * which save handshake context with server/client
         */
        handshake_packet handshake_pkt;
        /**
         * error function for internal callback
         */
        std::function<void(const std::error_code&)> error_func;
        /**
         * save connect callback
         */
        decltype(error_func) connect_func_slot;
        std::function<void(const control_packet_context&, const srt_packet&)> next_handshake_func;
        /**
         * receive buffer
         */
        mutable_basic_buffer<char> receive_buffer;
    };

    class srt_socket_helper{
        friend class socket_id;
    private:
        static uint32_t generator_socket_id();
        static bool insert_socket_id(uint32_t);
        static void destroy_socket_id(uint32_t);
        static std::set<uint32_t> socket_ids;
        static std::recursive_mutex mtx;
    };
};
#endif//TOOLKIT_SRT_CORE_HPP
