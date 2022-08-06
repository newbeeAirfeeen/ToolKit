/*
* @file_name: srt_core.cpp
* @date: 2022/04/26
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
#include "srt_core.hpp"
#include "Util/MD5.h"
#include "srt_error.hpp"
#include "srt_handshake.hpp"
#include <random>
namespace srt {
    srt_core::srt_core(event_poller &poller, asio::ip::udp::socket &sock, bool is_server)
        : poller(poller), sock(sock),
          ack_timer(poller.get_executor()),
          nak_timer(poller.get_executor()),
          keep_alive_timer(poller.get_executor()),
          connect_timer(poller.get_executor()),
          is_server(is_server){
        error_func = [](const std::error_code &e) {
            throw std::system_error(e);
        };
        current_seq_number = 0;
        if(is_server){
            next_handshake_func = std::bind(&srt_core::handle_handshake_induction_server_1, this, std::placeholders::_1, std::placeholders::_2);
        }
        else{
            next_handshake_func = std::bind(&srt_core::handle_handshake_induction_client_1, this, std::placeholders::_1, std::placeholders::_2);
        }
    }

    void srt_core::bind_local(const typename srt_core::endpoint_type &endpoint) {
        local_endpoint = endpoint;
    }

    void srt_core::set_connect_time_out(size_t miliseconds){
        connect_time = miliseconds * 1000;
    }

    void srt_core::set_mtu(uint32_t mtu_size){
        if( mtu_size > 1500 || mtu_size == 0){
            mtu_size = 1456;
        }
        handshake_pkt._max_mss = mtu_size;
    }

    void srt_core::set_window_size(uint32_t window_size){
        if(window_size == 0){
            window_size = 8192;
        }
        handshake_pkt._window_size = window_size;
    }

    void srt_core::register_error_function(const std::function<void(const std::error_code &)> &err_func) {
        if (err_func) {
            error_func = err_func;
        }
    }

    void srt_core::async_connect(const typename srt_core::endpoint_type &endpoint, const std::function<void(const std::error_code &)> &connect_func) {
        std::weak_ptr<srt_core> self(shared_from_this());
        /// update timestamp
        socket_start_time = steady_timer::clock_type::now();
        auto conn_func = [self, connect_func, endpoint]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            /**
            * srt divided into client and server, but only client is permitted to connect others.
            */
            if (stronger_self->is_server) {
                SRT_ERROR_LOG("when srt used for server, is not allowed to connect other");
                throw std::runtime_error("when srt used for server, is not allowed to connect other");
            }
            /**
            * connect operation is only permitted when in srt init status
            */
            if (stronger_self->_srt_status == srt_status::srt_init) {
                SRT_TRACE_LOG("srt open socket");
                stronger_self->sock.open(stronger_self->local_endpoint.protocol());
                stronger_self->_srt_status = srt_open;
            } else {
                if (stronger_self->_srt_status != srt_open) {
                    /**
                    * the srt_status not permitted which is not open status.
                    */
                    SRT_ERROR_LOG("srt_status not permitted which is not open status.");
                    throw srt_error(make_srt_error(srt_error_code::status_error));
                }
            }
            /**
             * when opened, change status to connecting
             */
            stronger_self->_srt_status = srt_connecting;
            stronger_self->sock.async_connect(endpoint,
                                              std::bind(&srt_core::async_connect_l, stronger_self, std::placeholders::_1, connect_func));
        };
        /**
         * switch to own thread.
         */
        poller.async(conn_func);
    }

    void srt_core::async_receive(buffer &buf, const std::function<void(size_t, const std::error_code &)> &receive_func) {

    }

    void srt_core::async_send(buffer &buf, const std::function<void(size_t, const std::error_code &)> &send_func) {

    }

    void srt_core::async_shutdown(const std::function<void(const std::error_code &)> &shutdown_func) {

    }

    void srt_core::close() {

    }

    void srt_core::read_internal() {
        /// begin loop
        /// if (is_first_receive) {
        ///    start_loop_timer();
        ///    is_first_receive = false;
        /// }
        auto stronger_self(shared_from_this());
        auto endpoint = std::make_shared<endpoint_type>();
        auto read_function = [stronger_self, endpoint](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->error_func(e);
                return;
            }
            stronger_self->receive_buffer.resize(length);
            stronger_self->onRecv(stronger_self->receive_buffer, *endpoint);
            stronger_self->read_internal();
        };
        stronger_self->receive_buffer.backward();
        stronger_self->receive_buffer.resize(2048);
        stronger_self->sock.async_receive_from(asio::buffer(stronger_self->receive_buffer.data(), stronger_self->receive_buffer.size()), *endpoint, read_function);
    }

    void srt_core::write_internal() {

    }
    /**
     * when invoke this function, the invoker is in own thread that no thread safe problem.
     */
    void srt_core::async_connect_l(const std::error_code &e, const std::function<void(const std::error_code &)> &connect_func) {
        connect_func_slot = connect_func;
        /**
         * if have any error, invoke registering function
         */
        if (e) {
            SRT_ERROR_LOG(e.message());
            return connect_func(e);
        }

        if(0){

        }
        else
        {
            /// for caller-listener configuration, set the version 4 for INDUCTION
            /// due to a serious problem in UDT code being also in the older SRT versions
            ///
            /// version
            handshake_pkt._version = 4;
            /// encryption field
            handshake_pkt.encryption = 0;
            /// extension field
            /// for UDT packet
            handshake_pkt.extension_field = 2;
        }
        /// sequence number
        handshake_pkt._sequence_number = generate_sequence_number();
        /// max transmission unit size
        if(!handshake_pkt._max_mss){
            handshake_pkt._max_mss = 1500;
        }
        /// maximum window size
        if(!handshake_pkt._window_size){
            handshake_pkt._window_size = 8196;
        }
        /// handshake type
        handshake_pkt._req_type = handshake_packet::packet_type::urq_induction;
        /// socket id
        handshake_pkt._socket_id = _sock_id.val();
        /// syn cookie for handshake
        handshake_pkt._cookie = 0;
        /// peer ip
        auto endpoint = sock.remote_endpoint();
        const unsigned char* source = nullptr;
        size_t address_length = 0;
        if(endpoint.address().is_v4()){
            source = endpoint.address().to_v4().to_bytes().data();
            address_length = 4;
        }
        else
        {
            source = endpoint.address().to_v6().to_bytes().data();
            address_length = 16;
            handshake_pkt.is_ipv6 = true;
        }
        unsigned char* begin = (unsigned char*)(&handshake_pkt._peer_ip[0]);
        std::copy(source, source + address_length, begin);
        /**
         * if no errors, send induction packet
         *
         * put induction into send queue.
         * begin read
         */
        this->read_internal();
        /// update socket start time
        send_connect_packet(handshake_pkt, true);
        loop();
    }

    void srt_core::start_loop_timer() {
        SRT_TRACE_LOG("start ack timer to send ack...");
        do_send_ack_op();
        SRT_TRACE_LOG("start nak timer to send nak...");
        do_send_nak_op();
    }

    void srt_core::handle_receive_queue_packets() {
        auto begin = receive_queue.begin();
        for (; begin != receive_queue.end(); ++begin) {
            handle_receive_data_packet(begin->second);
        }
        receive_queue.clear();
    }

    void srt_core::handle_receive_control_packet(const std::shared_ptr<srt_packet> &control_packet) {
        control_packet_context ctx(*control_packet);
        switch (ctx.get_control_type()) {
            case control_type::handshake:
                return handle_handshake(ctx, control_packet);
            case control_type::keepalive:
                return handle_keep_alive(control_packet);
            case control_type::ack:
                return handle_ack(control_packet);
            case control_type::nak:
                return handle_nak(control_packet);
            case control_type::congestion_warning:
                return handle_congestion_warn(control_packet);
            case control_type::shutdown:
                return handle_shutdown(control_packet);
            case control_type::ack_ack:
                return handle_ack_ack(control_packet);
            case control_type::dro_preq:
                return handle_dro_req(control_packet);
            case control_type::peer_error:
                return handle_peer_error(control_packet);
            case control_type::user_defined_type:
                return handle_user_defined_type(control_packet);
            default: {
                SRT_WARN_LOG("srt receive unknown packet control type which ignored");
            }
        }
    }


    void srt_core::handle_handshake(const control_packet_context& ctx, const std::shared_ptr<srt_packet>& handshake_packet) {
        switch (_srt_status) {
            /**
             * client handshake with server
             */
            case srt_status::srt_connecting:
            {
                next_handshake_func(ctx, *handshake_packet);
                break;
            }
            /**
             * listening server handshake with client
             */
            case srt_status::srt_listening:
                next_handshake_func(ctx, *handshake_packet);
                break;
            default: {
                SRT_ERROR_LOG("current srt status: {}, which is not permitted handshaking");
                throw srt_error(make_srt_error(status_error));
            }
        }
    }

    void srt_core::handle_handshake_induction_client_1(const control_packet_context& ctx, const srt_packet& handshake_packet){
        SRT_TRACE_LOG("received type=induction, sending back cookie+socket from server");
        do{
            /// cancel the connect timer
            this->connect_timer.cancel();

            bool loaded = handshake_pkt.load(ctx, handshake_packet);
            if(!loaded){
                SRT_ERROR_LOG("induction loaded error");
                break;
            }

            if(handshake_pkt._req_type != handshake_packet::packet_type::urq_induction){
                SRT_ERROR_LOG("handshake packet is not induction request type");
                break;
            }
            /**
             * page 37
             * if the party s srt, it does interpret the values in version and extension field.
             * if it receives value 5 in version, it understands that it comes from an SRT Party.
             * so it knows that it should prepare the proper handshake messages phase. it also checks
             * the following:
             *  1. whether the extension flags contains the magic value 0x4A17;
             *  otherwise the connection is rejected. this is a contingency for the case where someone who,
             *  in an attempt to extend UDT independently, increase the version value to 5 and tries to test
             *  against srt
             *
             *  2. whether the encryption flag contain a non-zero value, which is interpreted as an advertised cipher
             *  family and block size.
             */
            if(handshake_pkt._version < 5){
                SRT_ERROR_LOG("handshake version < 5");
                break;
            }
            /// magic value
            if(handshake_pkt.extension_field != 0x4A17){
                SRT_ERROR_LOG("handshake packet magic code != 0x4A17");
                break;
            }

            //check_update_crypto_key_len();
            /// for send back
            handshake_pkt._version = 5;
            handshake_pkt._req_type = handshake_packet::packet_type::urq_conclusion;
            /**
             * CONTROVERSIAL: use 0 as m_iType according to the meaning in HSv5.
             * The HSv4 client might not understand it, which means that agent
             * must switch itself to HSv4 rendezvous, and this time iType sould
             * be set to UDT_DGRAM value.
             */
            handshake_pkt.encryption = 0;
            ///when use version 5, must be set extension field.
            handshake_pkt.extension_field = 0;
            // create_crypter()

            //change
            next_handshake_func = std::bind(&srt_core::handle_handshake_conduction_client_2, this, std::placeholders::_1, std::placeholders::_2);
            /// send conduction.
            this->send_connect_packet(handshake_pkt, false);
            return;
        } while (0);
        /// handshake failed.
        this->send_reject();
    }

    void srt_core::handle_handshake_conduction_client_2(const control_packet_context& ctx, const srt_packet& handshake_packet){
        SRT_TRACE_LOG("received type=conduction, handshake completed");
        this->connect_timer.cancel();
        /// begin keep alive
        _srt_status = srt::srt_connected;
        do_send_keep_alive_op();
        /// success
        connect_func_slot(std::error_code());
    }


    void srt_core::handle_handshake_induction_server_1(const control_packet_context& ctx, const srt_packet& handshake_packet){
        if(handshake_pkt._req_type != handshake_packet_base::packet_type::urq_induction){
            SRT_ERROR_LOG("current srt connect, request type is not urq_induction");
            return;
        }
        SRT_TRACE_LOG("received type=induction, sending back cookie+socket");
        uint32_t cookie = 0;
        /**
         * calculate cookie
         */
        bake(cookie);
        SRT_TRACE_LOG("generate new cookie: {0:X}", cookie);
        handshake_pkt._cookie = cookie;
        /**
         * now's the time. The listener sets here the version 5 handshake,
         * even though the request was 4. This is because the old client would
         * simply return THE SAME version, not even looking into it, giving the
         * listener false impression as if it supported version 5.
         */
        handshake_pkt._version = 5;
        //handshake_pkt.type = ...
        uint32_t type_info_field = 0;
        auto l_to_c_handshake = srt_packet_helper::make_srt_control_packet(control_type::handshake,
                                                                         make_pkt_timestamp(),
                                                                           handshake_pkt._socket_id,
                                                                           type_info_field);
        /// version
        l_to_c_handshake->put_be(handshake_pkt._version);
        /// encryption field
        l_to_c_handshake->put_be(static_cast<unsigned short>(0));
        /// extension field
        l_to_c_handshake->put_be(static_cast<unsigned short>(0));
        /// sequence number
        l_to_c_handshake->put_be(handshake_pkt._sequence_number);
        /// mtu size
        l_to_c_handshake->put_be(handshake_pkt._max_mss);
        /// handshake window size
        l_to_c_handshake->put_be(handshake_pkt._window_size);
        /// handshake type
        l_to_c_handshake->put_be(static_cast<uint32_t>(handshake_packet_base::packet_type::urq_induction));
        /// srt socket id
        l_to_c_handshake->put_be(handshake_pkt._socket_id);
        /// cookie
        l_to_c_handshake->put_be(handshake_pkt._cookie);
        /// peer_ip
        l_to_c_handshake->append((const char*)handshake_pkt._peer_ip, 16);
        /// send srt packet
        send_srt_packet(l_to_c_handshake);
    }

    void srt_core::handle_keep_alive(const std::shared_ptr<srt_packet> &keep_alive_packet) {
        //SRT_TRACE_LOG("recv srt keep alive packet, sequence={}", keep_alive_packet->get_sequence());
    }

    void srt_core::handle_ack(const std::shared_ptr<srt_packet> &ack_packet) {
    }

    void srt_core::handle_nak(const std::shared_ptr<srt_packet> &nak_packet) {
    }

    void srt_core::handle_congestion_warn(const std::shared_ptr<srt_packet> &congestion_packet) {
    }

    void srt_core::handle_shutdown(const std::shared_ptr<srt_packet> &shutdown_packet) {
    }

    void srt_core::handle_ack_ack(const std::shared_ptr<srt_packet> &ack_ack_packet) {
    }

    void srt_core::handle_dro_req(const std::shared_ptr<srt_packet> &dro_req_packet) {
    }

    void srt_core::handle_peer_error(const std::shared_ptr<srt_packet> &peer_error_packet) {
    }

    void srt_core::handle_user_defined_type(const std::shared_ptr<srt_packet> &user_defined_packet) {
    }

    void srt_core::handle_receive_data_packet(const std::shared_ptr<srt_packet> &data_packet) {

    }

    void srt_core::send_connect_packet(const handshake_packet& pkt, bool init_ack_nak) {
        auto copy_pkt = std::make_shared<handshake_packet>(pkt);
        SRT_TRACE_LOG("put connect packet into send queue");
        /**
         * id = 0, connection request
         */
        auto connect_pkt = srt_packet_helper::make_srt_control_packet(control_type::handshake,
                                                                      make_pkt_timestamp(), 0, 0);
        auto handshake_buffer = handshake_packet::to_buffer(pkt);
        connect_pkt->append(handshake_buffer->data(), handshake_buffer->size());

        /// set initial ack
        if(init_ack_nak)
            set_initial_ack(handshake_pkt._sequence_number);
        /// send srt packet to peer
        send_srt_packet(connect_pkt);
        auto stronger_self(shared_from_this());
        /// set connect timeout for receive connect sent back.
        connect_timer.expires_after(std::chrono::milliseconds(250));
        connect_timer.async_wait([stronger_self, copy_pkt, init_ack_nak](const std::error_code& e){
            if(e){
                SRT_TRACE_LOG("connect timer manually triggered");
                return;
            }
            if( stronger_self->connect_time < 250 * 1000){
                return stronger_self->error_func(std::make_error_code(std::errc::connection_refused));
            }
            stronger_self->connect_time -= 250 * 1000;
            //send next connect packet.
            stronger_self->send_connect_packet(*copy_pkt, init_ack_nak);
        });
    }

    void srt_core::put_keep_alive_packet_into_send_queue() {
        SRT_TRACE_LOG("put_keep_alive_packet_into_send_queue");
    }

    /**
     * when invoke this function, the invoker is in own thread that no thread safe problem.
     */
    void srt_core::loop() {
        if (!is_in_loop) {
            is_in_loop = true;
            /**
             * save current timestamp for loop start
             */
            return loop_l();
        }
    }
    void srt_core::loop_l() {
        std::weak_ptr<srt_core> self(shared_from_this());
        /**
         * 1. receive packet from buffer
         *     * control packet
         *          ACK: send ACKACK
         *          ACKACK: calculate round-trip time
         *          NAK: send NAK packets immediately
         *          ...
         *          ...
         *     * data
         *
         * 2. put packet into recv queue.
         * 3. send ACK packets to send queue
         */
        handle_receive_queue_packets();
        // write to do write/send
        if (!send_queue.empty())
            do_write_op();

        /**
         * post loop function into next round-trip loop instead of recursive invoke,
         */

        poller.get_executor().post([self]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            stronger_self->loop_l();
        });
    }

    /**
     * Page 37
     * a cookie that is crafted based on host, port and
     * current time with 1 minute accuracy to avoid SYN flooding attack
     */
    void srt_core::bake(uint32_t& current_cookie){
        auto old_cookie = current_cookie;
        int index = 0;
        auto timestamp = std::chrono::duration_cast<std::chrono::minutes>(steady_timer::clock_type::now() - socket_start_time);
        for(;;){
            std::string cookie_str = sock.remote_endpoint().address().to_string() + ":" + std::to_string(sock.remote_endpoint().port()) + ":" + std::to_string(timestamp.count() + index);
            SRT_INFO_LOG("handshake bake result: {}", cookie_str);
            toolkit::MD5_digest md5(cookie_str);
            std::string raw_digest = md5.rawdigest();
            current_cookie = *(uint32_t*)raw_digest.data();
            /**
             * guarantee current_cookie is different from old cookie
             */
            if(current_cookie != old_cookie){
                break;
            }
            ++index;
        }
    }

    void srt_core::do_write_op() {
        auto stronger_self(shared_from_this());
        std::shared_ptr<void> visitor(nullptr, [this](void *) {
            this->last_any_packet_sent = steady_timer::clock_type::now();
            this->do_send_keep_alive_op();
        });
        for (const auto &item: send_queue) {
            auto packet_ref = item.second;
            sock.async_send(asio::buffer(item.second->data(), item.second->size()),
                            [stronger_self, packet_ref, visitor](const std::error_code &e, size_t length) {
                if (e) {
                    /**
                          * if write error, cancel all operation for socket.
                          */
                    stronger_self->sock.cancel();
                    return stronger_self->error_func(e);
                }
                //when send packet, save packet_ref in send_lost_queue

                //stronger_self->send_lost_queue[packet_ref->get_sequence()] = std::move(packet_ref); });
                    });
        }
        //clear send_queue
        send_queue.clear();
    }

    void srt_core::do_send_ack_op() {
        static std::chrono::milliseconds ack_duration(ack_interval);
        auto stronger_self(shared_from_this());
        ack_timer.expires_after(ack_duration);
        ack_timer.async_wait([stronger_self](const std::error_code &e) {
            if (e) {
                SRT_ERROR_LOG(e.message());
                return;
            }
            stronger_self->do_send_ack_op_l();
            stronger_self->do_send_ack_op();
        });
    }

    void srt_core::do_send_ack_op_l() {
        SRT_TRACE_LOG("send 10 ms ack...");
    }

    void srt_core::do_send_nak_op() {
        /**
         * 4.8.2 p53
         * SRT Periodic NAK reports are sent with a period of (RTT + 4 * RTTVar) / 2 (so called NAKInterval),
         * minimum time is 20ms.
         */
        nak_interval = static_cast<size_t>((RTT + 4 * RTTVar) / 2);
        nak_interval = nak_interval > 20 ? nak_interval : 20;
        std::chrono::milliseconds nak_duration(nak_interval);
        auto stronger_self(shared_from_this());
        ack_timer.expires_after(std::chrono::milliseconds(nak_duration));
        ack_timer.async_wait([stronger_self](const std::error_code &e) {
            if (e) {
                SRT_ERROR_LOG(e.message());
                return;
            }
            stronger_self->do_send_nak_op_l();
            stronger_self->do_send_nak_op();
        });
    }

    void srt_core::do_send_nak_op_l() {
        SRT_TRACE_LOG("send {} ms nak...", nak_interval);
    }

    void srt_core::do_send_keep_alive_op() {
        static std::chrono::milliseconds keep_alive_duration(keep_alive_internal);
        /// do not send in other status
        if(_srt_status == srt_status::srt_connected){
            return;
        }
        auto stronger_self(shared_from_this());
        keep_alive_timer.expires_after(keep_alive_duration);
        keep_alive_timer.async_wait([stronger_self](const std::error_code &e) {
            /**
             * If triggered manually, don't send keep alive to peer. otherwise, auto triggered time
             * out sending event -> send keep alive.
             */
            if (e){
                SRT_TRACE_LOG(e.message());
            }
            else
                stronger_self->do_send_keep_alive_op_l();
            stronger_self->do_send_keep_alive_op();
        });
    }

    void srt_core::do_send_keep_alive_op_l() {
        /**
         * Keep-alive control packets are sent after a certain timeout from the
         * last time any packet (Control or Data) was sent. The purpose of this
         * control packet is to notify the peer to keep the connection open when
         * no data exchange is taking place.
         */
        SRT_TRACE_LOG("send {} ms keep_alive...", keep_alive_internal);
        if (last_any_packet_sent == steady_timer::clock_type::time_point::min())
            return;
        auto send_packet_time_duration = steady_timer::clock_type::now() - last_any_packet_sent;
        auto real_duration = std::chrono::duration_cast<std::chrono::duration<double>>(send_packet_time_duration);
        SRT_TRACE_LOG("keep_alive_internal={} ms, real_duration={}s", keep_alive_internal, real_duration.count());
        if (real_duration.count() >= static_cast<double>(keep_alive_internal))
            put_keep_alive_packet_into_send_queue();
    }

    void srt_core::set_initial_ack(uint32_t seq){
        last_ack_send_seq = seq;
        last_full_ack_recv_seq = seq;
        ack_current_send_seq =  seq == 0 ? 0x7FFFFFFF : (seq - 1);
        next_ack_send_seq = seq;
        last_ack_send_back = seq;
        /// save ack sent back time_point
        last_ack_sent_back = steady_timer::clock_type::now();
    }

    inline uint32_t srt_core::make_pkt_timestamp(){
        auto micro_interval =
                std::chrono::duration_cast<std::chrono::microseconds>(steady_timer::clock_type::now() - socket_start_time);
        return static_cast<uint32_t>(micro_interval.count());
    }

    inline uint32_t srt_core::generate_sequence_number(){
        std::default_random_engine generator(static_cast<uint32_t>(::time(nullptr)));
        std::uniform_int_distribution<uint32_t> distribution(0,0x3FFFFFFF);
        return distribution(generator);
    }

    void srt_core::send_reject(){

    }

    void srt_core::send_srt_packet(const std::shared_ptr<srt_packet>& pkt, bool after_send_keep_alive){
        auto stronger_self(shared_from_this());
        auto func = [stronger_self, pkt, after_send_keep_alive](const std::error_code& e, size_t length){
            if (e) {
                SRT_ERROR_LOG("send error: {}", e.message());
                stronger_self->sock.close();
                return;
            }
            /// send keep alive op
            stronger_self->last_any_packet_sent = steady_timer::clock_type::now();
            if(after_send_keep_alive){
                stronger_self->do_send_keep_alive_op();
            }
        };
        this->sock.async_send(asio::buffer(pkt->data(), pkt->size()), func);
    }

    void srt_core::onRecv(basic_buffer<char>& buf, const endpoint_type& endpoint){
        auto srt_pkt = std::make_shared<srt_packet>(std::move(buf));
        if(!srt_pkt->size()){
            return;
        }
        if(srt_pkt->is_control_packet()){
            return handle_receive_control_packet(srt_pkt);
        }
        handle_receive_data_packet(srt_pkt);
    }

    //// for generator socket id and destroy
    std::set<uint32_t> srt_socket_helper::socket_ids;
    std::recursive_mutex srt_socket_helper::mtx;

    socket_id::socket_id():value(srt_socket_helper::generator_socket_id()){}

    socket_id::~socket_id(){
        srt_socket_helper::destroy_socket_id(value);
    }

    bool socket_id::reset(uint32_t value){
        if(this->value == value){
            return value;
        }
        return srt_socket_helper::insert_socket_id(value);
    }

    uint32_t socket_id::val() const{
        return this->value;
    }

    uint32_t srt_socket_helper::generator_socket_id(){
        static std::default_random_engine generator;
        auto generator_func = [&]() -> uint32_t {
            std::uniform_int_distribution<uint32_t> distribution(0,(std::numeric_limits<uint32_t>::max)());
            return distribution(generator);
        };
        for(;;){
            auto socket_id_ = generator_func();
            {
                std::lock_guard<std::recursive_mutex> lmtx(srt_socket_helper::mtx);
                auto it = socket_ids.find(socket_id_);
                if(it == socket_ids.end()){
                    return socket_id_;
                }
            }
        }
    }

    bool srt_socket_helper::insert_socket_id(uint32_t val){
        {
            std::lock_guard<std::recursive_mutex> lmtx(srt_socket_helper::mtx);
            auto it = socket_ids.find(val);
            if(it == socket_ids.end()){
                socket_ids.insert(val);
                return true;
            }
        }
        return false;
    }

    void srt_socket_helper::destroy_socket_id(uint32_t val){
        std::lock_guard<std::recursive_mutex> lmtx(srt_socket_helper::mtx);
        socket_ids.erase(val);
    }
};// namespace srt