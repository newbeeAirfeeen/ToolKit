﻿/*
* @file_name: srt_socket.cpp
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
#include "srt_socket_service.hpp"
#include "spdlog/logger.hpp"
#include "srt_error.hpp"
#include "srt_extension.h"
#include "srt_handshake.h"
#include "srt_packet.h"
#include <chrono>
#include <random>
#include <numeric>
namespace srt {
    /// 连接回调, 发生在握手
    enum timer_expired_type {
        keep_alive_expired,
        induction_expired,
        conclusion_expired,
    };

    srt_socket_service::srt_socket_service(asio::io_context &executor) : poller(executor) {

    }

    void srt_socket_service::begin(){
        //// 创建common 定时器
        common_timer = create_deadline_timer<int>(poller);
        std::weak_ptr<srt_socket_service> self(shared_from_this());
        common_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (stronger_self){
                stronger_self->on_common_timer_expired(v);
            }
        });

        keep_alive_timer = create_deadline_timer<int>(poller);
        keep_alive_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            stronger_self->on_keep_alive_expired(v);
        });
    }

    void srt_socket_service::connect() {
        Trace("srt socket begin connect to {}:{}", get_remote_endpoint().address().to_string(), get_remote_endpoint().port());
        std::default_random_engine random(std::random_device{}());
        std::uniform_int_distribution<int32_t> mt(0, (std::numeric_limits<int32_t>::max)());

        handshake_context ctx;
        ctx._version = 4;
        ctx.encryption = 0;
        /// DGRAM
        ctx.extension_field = 2;
        /// random seq
        sequence_number = static_cast<uint32_t>(mt(random));
        ctx._sequence_number = sequence_number;
        ctx._max_mss = srt_socket_base::get_max_payload();
        ctx._window_size = srt_socket_service::get_max_flow_window_size();
        ctx._req_type = srt::handshake_context::urq_induction;
        // random
        ctx._socket_id = static_cast<uint32_t>(mt(random));
        ctx._cookie = static_cast<uint32_t>(mt(random));
        ctx.address = get_local_endpoint().address();

        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        srt_packet pkt;
        pkt.set_control(true);
        pkt.set_control_type(control_type::handshake);
        pkt.set_timestamp(static_cast<uint32_t>(ts));
        pkt.set_socket_id(0);
        pkt.set_type_information(0);
        /// srt_packet

        auto _pkt = create_packet(pkt);
        handshake_context::to_buffer(ctx, _pkt);
        /// save induction message
        handshake_buffer = _pkt;
        _next_func = bind(&srt_socket_service::handle_server_induction, this, std::placeholders::_1);
        /// 发送到对端
        send_in(handshake_buffer, get_remote_endpoint());
        /// 开始定时器, 每隔一段时间发送induction包
        first_connect_point = clock_type::now();
        this->common_timer->add_expired_from_now(250, timer_expired_type::induction_expired);
    }

    void srt_socket_service::on_common_timer_expired(const int &val) {
        int v = val;
        switch (v) {
            case timer_expired_type::induction_expired:
            case timer_expired_type::conclusion_expired: {
                //Trace("connect package may be lost, try send again, package type={}", v);
                //// 连接超时
                if (std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - first_connect_point).count() > get_connect_timeout()) {
                    return on_error(make_srt_error(srt_error_code::socket_connect_time_out));
                }
                /// 更新包的时间戳
                auto now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                set_packet_timestamp(handshake_buffer, now);
                send(handshake_buffer, get_remote_endpoint());
                /// 260ms后尝试重新发送
                this->common_timer->add_expired_from_now(250, val);
                break;
            }
        }
    }

    void srt_socket_service::on_keep_alive_expired(const int &v) {
        /// 如果没有连接 停止发送
        if (!is_connected) {
            return;
        }
        auto ts = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now().time_since_epoch()).count());
        if (!keep_alive_buffer) {
            srt_packet pkt;
            pkt.set_control(true);
            pkt.set_control_type(control_type::keepalive);
            pkt.set_type_information(0);
            pkt.set_socket_id(srt_socket_base::get_sock_id());
            pkt.set_timestamp(ts);
            keep_alive_buffer = create_packet(pkt);
        } else {
            /// 更新时间戳
            set_packet_timestamp(keep_alive_buffer, ts);
        }
        send_in(keep_alive_buffer, get_remote_endpoint());
    }

    void srt_socket_service::input_packet(const std::shared_ptr<buffer> &buff) {
        _next_func(buff);
    }

    void srt_socket_service::send_reject() {
        Trace("send_reject..");
    }

    void srt_socket_service::send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) {
        send(buff, where);
        /// 更新 上一次发送的时间
        last_send_point = clock_type::now();
        /// 发送keepalive
        do_keepalive();
    }

    void srt_socket_service::do_keepalive() {
        if (!is_connected) {
            return;
        }
        keep_alive_timer->stop();
        /// 一秒一次
        keep_alive_timer->add_expired_from_now(1000, keep_alive_expired);
    }


    void srt_socket_service::handle_server_induction(const std::shared_ptr<buffer> &buff) {
        Trace("receive server induction..");
        try {
            auto induction_pkt = srt::from_buffer(buff->data(), 16);
            if (!induction_pkt) {
                Warn("srt packet parse error");
                return;
            }

            if (induction_pkt->get_control_type() != control_type::handshake) {
                Warn("srt packet control type is not handshake");
                return;
            }

            buff->remove(16);
            auto induction_context = srt::handshake_context::from_buffer(buff->data(), buff->size());
            if (!induction_context) {
                Warn("handshake_context parse error");
                return;
            }
            /// 非法的握手包
            if (induction_context->_req_type != handshake_context::urq_induction) {
                Warn("response is not urq_induction type");
                return;
            }
            //// 更新参数
            if (induction_context->_version != 5) {
                /// 握手版本不正确
                return send_reject();
            }
            /// 版本5的扩展字段检查
            if (induction_context->extension_field != 0x4A17) {
                return send_reject();
            }

            /// 暂不支持加密
            if (induction_context->encryption != 0) {
                return send_reject();
            }

            /// mtu 检查
            if (induction_context->_max_mss > 1500) {
                return send_reject();
            }
            //// 停止定时器
            common_timer->stop();

            //// 更新包序号
            sequence_number = induction_context->_sequence_number;
            /// 更新mtu
            set_max_payload(induction_context->_max_mss);
            /// 更新滑动窗口大小
            set_max_flow_window_size(induction_context->_window_size);
            /// 更新socket_id
            set_sock_id(induction_context->_socket_id);

            handshake_context ctx;
            ctx._version = 5;
            ctx.encryption = 0;
            /// 先为空
            ctx.extension_field = 0;
            ctx._sequence_number = sequence_number;
            ctx._max_mss = srt_socket_base::get_max_payload();
            ctx._window_size = srt_socket_service::get_max_flow_window_size();
            ctx._req_type = srt::handshake_context::urq_conclusion;
            // random
            ctx._socket_id = get_sock_id();
            ctx._cookie = induction_context->_cookie;
            ctx.address = get_local_endpoint().address();

            auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            srt_packet pkt;
            pkt.set_control(true);
            pkt.set_control_type(control_type::handshake);
            pkt.set_timestamp(static_cast<uint32_t>(ts));
            pkt.set_socket_id(0);
            pkt.set_type_information(0);
            /// srt_packet
            auto _pkt = create_packet(pkt);
            /// 加入握手
            handshake_context::to_buffer(ctx, _pkt);
            /// 加入扩展字段
            set_extension(ctx, _pkt, get_time_based_deliver(), get_drop_too_late_packet(), get_report_nak(), get_stream_id());
            /// 最后更新extension_field
            handshake_context::update_extension_field(ctx, _pkt);
            /// 更新握手上下文
            _next_func = std::bind(&srt_socket_service::handle_server_conclusion, this, std::placeholders::_1);
            /// 保存握手缓存
            handshake_buffer = _pkt;
            send_in(_pkt, get_remote_endpoint());
            common_timer->add_expired_from_now(250, conclusion_expired);
        } catch (const std::system_error &e) {
            return on_error(e.code());
        }
    }

    /// conclusion receive
    void srt_socket_service::handle_server_conclusion(const std::shared_ptr<buffer> &buff) {
        Trace("receive server conclusion..");
        try{
            auto size = buff->size();
            auto pkt = srt::from_buffer(buff->data(), buff->size());
            if(!pkt){

            }
            auto context = srt::handshake_context::from_buffer(buff->data(), buff->size());
            if(!context){
                throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
            }

            buff->remove(48);
            auto extension = get_extension(*context, buff);

            set_time_based_deliver(extension->receiver_tlpktd_delay);
            set_drop_too_late_packet(extension->drop);
            set_report_nak(extension->nak);
            Trace("srt handshake success, drop={}, report_nak={}, tsbpd={}", extension->drop, extension->nak, extension->receiver_tlpktd_delay);
            /// 停止计时器
            common_timer->stop();
            is_connected = true;
            /// 清除缓存, 节省内存
            handshake_buffer = nullptr;
            _next_func = std::bind(&srt_socket_service::handle_data_and_control, this, std::placeholders::_1);
            /// 开启keepalive
            do_keepalive();
        }
        catch(const std::system_error& e){
            send_reject();
            return on_error(e.code());
        }
    }


    void srt_socket_service::handle_data_and_control(const std::shared_ptr<buffer>& buff){

    }
};// namespace srt
