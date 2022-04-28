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
#include "err.hpp"

#ifdef SRT_CORE_INTERNAL_DEBUG_LOG
    #define SRT_TRACE_LOG(...) Trace(__VA_ARGS__)
    #define SRT_DEBUG_LOG(...) Debug(__VA_ARGS__)
    #define SRT_INFO_LOG(...)  Info(__VA_ARGS__)
    #define SRT_WARN_LOG(...)  Warn(__VA_ARGS__)
    #define SRT_ERROR_LOG(...) Error(__VA_ARGS__)
#else
    #define SRT_TRACE_LOG(...)
    #define SRT_DEBUG_LOG(...)
    #define SRT_INFO_LOG(...)
    #define SRT_WARN_LOG(...)
    #define SRT_ERROR_LOG(...)
#endif


namespace srt{
    srt_core::srt_core(event_poller& poller, asio::ip::udp::socket& sock,bool is_server)
        :poller(poller),sock(sock),
          ack_timer(poller.get_executor()),
          nak_timer(poller.get_executor()),
          keep_alive_timer(poller.get_executor()),
          is_server(is_server){
        error_func = [](const std::error_code& e){
            throw std::system_error(e);
        };
        current_seq_number = 0;
    }

    void srt_core::bind_local(const typename srt_core::endpoint_type& endpoint){
        local_endpoint = endpoint;
    }

    void srt_core::register_error_function(const std::function<void(const std::error_code&)>& err_func){
        if(err_func){
            error_func = err_func;
        }
    }

    void srt_core::async_connect(const typename srt_core::endpoint_type& endpoint, const std::function<void(const std::error_code&)>& connect_func){
        std::weak_ptr<srt_core> self(shared_from_this());
        auto conn_func = [self, connect_func, endpoint](){
            auto stronger_self = self.lock();
            if(!stronger_self){
                return;
            }
            /**
            * srt divided into client and server, but only client is permitted to connect others.
            */
            if(stronger_self->is_server){
                SRT_ERROR_LOG("when srt used for server, is not allowed to connect other");
                throw std::runtime_error("when srt used for server, is not allowed to connect other");
            }
            /**
            * connect operation is only permitted when in srt init status
            */
            if(stronger_self->_srt_status == srt_status::srt_init){
                SRT_TRACE_LOG("srt open socket");
                stronger_self->sock.open(stronger_self->local_endpoint.protocol());
                stronger_self->_srt_status = srt_open;
            }else{
                if(stronger_self->_srt_status != srt_open){
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

    void srt_core::async_receive(buffer& buf, const std::function<void(size_t, const std::error_code&)>& receive_func){

    }

    void srt_core::async_send(buffer& buf, const std::function<void(size_t, const std::error_code&)>& send_func){

    }

    void srt_core::async_shutdown(const std::function<void(const std::error_code&)>& shutdown_func){

    }

    void srt_core::close(){

    }

    void srt_core::begin_transaction(){
        /**
         * begin read
         */
        read_internal();
    }

    void srt_core::read_internal(){

    }

    void srt_core::write_internal(){

    }
    /**
     * when invoke this function, the invoker is in own thread that no thread safe problem.
     */
    void srt_core::async_connect_l(const std::error_code& e, const std::function<void(const std::error_code&)>& connect_func){
        /**
         * if have any error, invoke registering function
         */
        if(e){
            SRT_ERROR_LOG(e.message());
            return connect_func(e);
        }
        /**
         * if no errors, send induction packet
         *
         * put induction into send queue.
         */
        put_connect_packet_into_send_queue();
        loop();
    }

    void srt_core::start_loop_timer(){
        SRT_TRACE_LOG("start ack timer to send ack...");
        do_send_ack_op();
        SRT_TRACE_LOG("start nak timer to send nak...");
        do_send_nak_op();
    }

    void srt_core::handle_receive_queue_packets(){
        auto begin = receive_queue.begin();
        for(; begin != receive_queue.end(); ++begin){
            if(is_first_receive){
                start_loop_timer();
                is_first_receive = false;
            }
            if(begin->second->is<control_packet>(begin->second->get_packet_header()))
                handle_receive_control_packet(begin->second);
            else
                handle_receive_data_packet(begin->second);
        }
        receive_queue.clear();
    }

    void srt_core::handle_receive_control_packet(const std::shared_ptr<srt_packet>& control_packet){
        control_type packet_type = srt_packet_helper::get_control_packet_type(control_packet->get_packet_header());
        switch (packet_type) {
            case control_type::handshake: return handle_handshake(control_packet);
            case control_type::keepalive: return handle_keep_alive(control_packet);
            case control_type::ack: return handle_ack(control_packet);
            case control_type::nak: return handle_nak(control_packet);
            case control_type::congestion_warning: return handle_congestion_warn(control_packet);
            case control_type::shutdown: return handle_shutdown(control_packet);
            case control_type::ack_ack:  return handle_ack_ack(control_packet);
            case control_type::dro_preq: return handle_dro_req(control_packet);
            case control_type::peer_error: return handle_peer_error(control_packet);
            case control_type::user_defined_type: return handle_user_defined_type(control_packet);
            default:{
                SRT_WARN_LOG("srt receive unknown packet control type which ignored");
            }
        }
    }


    void srt_core::handle_handshake(const std::shared_ptr<srt_packet>& handshake_packet){
        SRT_TRACE_LOG("srt status: {}", (int)_srt_status);
        switch (_srt_status) {
            /**
             * client handshake with server
             */
            case srt_status::srt_connecting:{

                break;
            }
            /**
             * listening server handshake with client
             */
            case srt_status::srt_listening:{

                break;
            }
            default:{
                SRT_ERROR_LOG("current srt status: {}, which is not permitted handshaking");
            }
        }
    }

    void srt_core::handle_keep_alive(const std::shared_ptr<srt_packet>& keep_alive_packet){


    }

    void srt_core::handle_ack(const std::shared_ptr<srt_packet>& ack_packet){

    }

    void srt_core::handle_nak(const std::shared_ptr<srt_packet>& nak_packet){


    }

    void srt_core::handle_congestion_warn(const std::shared_ptr<srt_packet>& congestion_packet){


    }

    void srt_core::handle_shutdown(const std::shared_ptr<srt_packet>& shutdown_packet){


    }

    void srt_core::handle_ack_ack(const std::shared_ptr<srt_packet>& ack_ack_packet){


    }

    void srt_core::handle_dro_req(const std::shared_ptr<srt_packet>& dro_req_packet){


    }

    void srt_core::handle_peer_error(const std::shared_ptr<srt_packet>& peer_error_packet){


    }

    void srt_core::handle_user_defined_type(const std::shared_ptr<srt_packet>& user_defined_packet){


    }

    void srt_core::handle_receive_data_packet(const std::shared_ptr<srt_packet>& data_packet){

    }

    void srt_core::put_connect_packet_into_send_queue(){
        SRT_TRACE_LOG("put connect packet into send queue");

    }

    void srt_core::put_keep_alive_packet_into_send_queue(){
        SRT_TRACE_LOG("put_keep_alive_packet_into_send_queue");
    }

    /**
     * when invoke this function, the invoker is in own thread that no thread safe problem.
     */
    void srt_core::loop(){
        if(!is_in_loop){
            is_in_loop = true;
            return loop_l();
        }
    }
    void srt_core::loop_l(){
        auto stronger_self(shared_from_this());

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
        do_write_op();
        /**
         * post loop function into next round-trip loop instead of recursive invoke,
         */
        poller.get_executor().post([stronger_self](){
            stronger_self->loop_l();
        });
    }

    void srt_core::do_read_op(){

    }

    void srt_core::do_write_op(){
        auto stronger_self(shared_from_this());
        std::shared_ptr<void> visitor(nullptr, [this](void*){
            this->last_any_packet_sent = steady_timer::clock_type::now();
            this->do_send_keep_alive_op();
        });
        for(const auto& item : send_queue){
            auto packet_ref = item.second;
            sock.async_send(asio::buffer(item.second->data(), item.second->size()),
                            [stronger_self, packet_ref, visitor](const std::error_code& e, size_t length){
                if(e){
                    /**
                     * if write error, cancel all operation for socket.
                     */
                    stronger_self->sock.cancel();
                    return stronger_self->error_func(e);
                }
                //when send packet, save packet_ref in send_lost_queue
                stronger_self->send_lost_queue[packet_ref->get_sequence()] = std::move(packet_ref);
            });
        }
        //clear send_queue
        send_queue.clear();
    }

    void srt_core::do_send_ack_op(){
        static std::chrono::milliseconds ack_duration(ack_interval);
        auto stronger_self(shared_from_this());
        ack_timer.expires_after(ack_duration);
        ack_timer.async_wait([stronger_self](const std::error_code& e){
            if(e){
                SRT_ERROR_LOG(e.message());
                return;
            }
            stronger_self->do_send_ack_op_l();
            stronger_self->do_send_ack_op();
        });
    }

    void srt_core::do_send_ack_op_l(){
        SRT_TRACE_LOG("send 10 ms ack...");

    }

    void srt_core::do_send_nak_op(){
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
        ack_timer.async_wait([stronger_self](const std::error_code& e){
            if(e){
                SRT_ERROR_LOG(e.message());
                return;
            }
            stronger_self->do_send_nak_op_l();
            stronger_self->do_send_ack_op();
        });
    }

    void srt_core::do_send_nak_op_l(){
        SRT_TRACE_LOG("send {} ms nak...", nak_interval);

    }

    void srt_core::do_send_keep_alive_op(){
        static std::chrono::milliseconds keep_alive_duration(keep_alive_internal);
        auto stronger_self(shared_from_this());
        keep_alive_timer.expires_after(keep_alive_duration);
        keep_alive_timer.async_wait([stronger_self](const std::error_code& e){
            /**
             * If triggered manually, don't send keep alive to peer. otherwise, auto triggered time
             * out sending event -> send keep alive.
             */
            if(e)
                SRT_TRACE_LOG(e.message());
            else
                stronger_self->do_send_keep_alive_op_l();
            stronger_self->do_send_keep_alive_op();
        });
    }

    void srt_core::do_send_keep_alive_op_l(){
        /**
         * Keep-alive control packets are sent after a certain timeout from the
         * last time any packet (Control or Data) was sent. The purpose of this
         * control packet is to notify the peer to keep the connection open when
         * no data exchange is taking place.
         */
        SRT_TRACE_LOG("send {} ms keep_alive...", keep_alive_internal);
        if(last_any_packet_sent == steady_timer::clock_type::time_point::min())
            return;
        auto send_packet_time_duration =  steady_timer::clock_type::now() - last_any_packet_sent;
        auto real_duration = std::chrono::duration_cast<std::chrono::duration<double>>(send_packet_time_duration);
        SRT_TRACE_LOG("keep_alive_internal={} ms, real_duration={}s", keep_alive_internal ,real_duration.count());
        if(real_duration.count() >= static_cast<double>(keep_alive_internal))
            put_keep_alive_packet_into_send_queue();
    }
};