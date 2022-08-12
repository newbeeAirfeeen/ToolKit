/*
* @file_name: srt_socket.cpp
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
#include "srt_socket_service.hpp"
#include "Util/endian.hpp"
#include "spdlog/logger.hpp"
#include "srt_error.hpp"
#include "srt_extension.h"
#include "srt_handshake.h"
#include "srt_packet.h"
#include <chrono>
#include <numeric>
#include <random>
namespace srt {
    static constexpr uint32_t packet_max_seq = 0x7FFFFFFF;
    static constexpr uint32_t message_max_seq = 0x3FFFFFF;
    /// 连接回调, 发生在握手
    enum timer_expired_type {
        keep_alive_expired,
        induction_expired,
        conclusion_expired,
        receive_timeout,
    };

    srt_socket_service::srt_socket_service(asio::io_context &executor) : poller(executor), sender_queue<buffer_pointer>(executor) {
        _sock_send_statistic = std::make_shared<socket_statistic>(poller);
        _sock_receive_statistic = std::make_shared<socket_statistic>(poller);
    }

    void srt_socket_service::begin() {
        //// 创建common 定时器
        common_timer = create_deadline_timer<int>(poller);
        std::weak_ptr<srt_socket_service> self(shared_from_this());
        common_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (stronger_self) {
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
        packet_sequence_number = static_cast<uint32_t>(mt(random));
        ctx._sequence_number = packet_sequence_number;
        ctx._max_mss = srt_socket_base::get_max_payload();
        ctx._window_size = srt_socket_service::get_max_flow_window_size();
        ctx._req_type = srt::handshake_context::urq_induction;
        // random
        ctx._socket_id = static_cast<uint32_t>(mt(random));
        ctx._cookie = static_cast<uint32_t>(mt(random));
        ctx.address = get_local_endpoint().address();

        /// auto ts = std::chrono::duration_cast<std::chrono::microseconds>
        /// (std::chrono::steady_clock::now().time_since_epoch()).count();

        srt_packet pkt;
        pkt.set_control(true);
        pkt.set_control_type(control_type::handshake);
        pkt.set_timestamp(0);
        pkt.set_socket_id(0);
        pkt.set_type_information(0);
        /// srt_packet

        auto _pkt = create_packet(pkt);
        handshake_context::to_buffer(ctx, _pkt);
        /// save induction message
        handshake_buffer = _pkt;
        first_connect_point = clock_type::now();
        _next_func = bind(&srt_socket_service::handle_server_induction, this, std::placeholders::_1);
        /// 发送到对端
        send_in(handshake_buffer, get_remote_endpoint());
        /// 开始定时器, 每隔一段时间发送induction包
        this->common_timer->add_expired_from_now(250, timer_expired_type::induction_expired);
        /// 套接字已经打开
        _is_open.store(true, std::memory_order_relaxed);
    }

    void srt_socket_service::on_common_timer_expired(const int &val) {
        int v = val;
        switch (v) {
            case timer_expired_type::induction_expired:
            case timer_expired_type::conclusion_expired: {
                //Trace("connect package may be lost, try send again, package type={}", v);
                //// 连接超时
                auto ts = get_time_from_begin();
                if (ts > get_connect_timeout()) {
                    return on_error_in(make_srt_error(srt_error_code::socket_connect_time_out));
                }
                /// 更新包的时间戳,当前时间减去第一次连接的时间
                set_packet_timestamp(handshake_buffer, ts);
                send(handshake_buffer, get_remote_endpoint());
                /// 统计包丢失
                /// _sock_send_statistic->report_packet_lost(1);
                /// 260ms后尝试重新发送
                this->common_timer->add_expired_from_now(250, val);
                break;
            }
            /// 接收超时 断开连接
            case timer_expired_type::receive_timeout: {
                /// 上一次接收到到时间点
                uint32_t leave_last_receive_time_point = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - last_receive_point).count();
                if (leave_last_receive_time_point > srt_socket_service::max_receive_time_out) {
                    do_shutdown();
                    return on_error_in(make_srt_error(srt_error_code::lost_peer_connection));
                } else {
                    common_timer->add_expired_from_now(srt_socket_service::max_receive_time_out - leave_last_receive_time_point, timer_expired_type::receive_timeout);
                }
            }
        }
    }

    void srt_socket_service::on_keep_alive_expired(const int &v) {
        /// 如果没有连接 停止发送
        if (!_is_connected.load(std::memory_order_relaxed)) {
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

    inline uint32_t srt_socket_service::get_next_packet_sequence_number() {
        auto tmp = packet_sequence_number;
        packet_sequence_number = (packet_sequence_number + 1) % packet_max_seq;
        return tmp;
    }

    inline uint32_t srt_socket_service::get_next_packet_message_number() {
        auto tmp = message_number;
        message_number = (message_max_seq + 1) % message_max_seq;
        return tmp;
    }

    void srt_socket_service::input_packet(const std::shared_ptr<buffer> &buff) {
        _next_func(buff);
    }

    bool srt_socket_service::is_open() {
        return _is_open.load(std::memory_order_relaxed);
    }

    bool srt_socket_service::is_connected() {
        return _is_connected.load(std::memory_order_relaxed);
    }

    void srt_socket_service::async_send(const std::shared_ptr<buffer> &buffer) {
        std::weak_ptr<srt_socket_service> self(shared_from_this());
        poller.post([self, buffer]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }

            if (!stronger_self->_is_connected.load(std::memory_order_relaxed)) {
                auto e = make_srt_error(srt_error_code::not_connected_yet);
                /// 在连接的时候 直接调用.不终止会话loop
                return stronger_self->on_error(e);
            }
        });
    }

    using pointer = typename sender_queue<buffer_pointer>::pointer;
    pointer srt_socket_service::get_shared_from_this() {
        return shared_from_this();
    }

    void srt_socket_service::send(const sender_queue::block_type &type) {
        /// real send
        send_in(type->content, get_remote_endpoint());
    }

    void srt_socket_service::on_drop_packet(sender_queue::size_type type) {
        Warn("drop packet,{}", type);
    }


    void srt_socket_service::send_reject(int e, const std::shared_ptr<buffer> &buff) {
        Trace("send_reject..");
        if (buff->size() < 40) {
            throw std::invalid_argument("buff is MUST greater than 40");
        }
        /// 停止重新发送定时器
        common_timer->stop();
        ////
        buff->backward();
        auto *p = (uint32_t *) (buff->data() + 36);
        set_be32(p, static_cast<uint32_t>(e));
        auto reject_buf = std::make_shared<buffer>(buff->data(), buff->size());
        send_in(reject_buf, get_remote_endpoint());
        /// 最后调用on_error
        on_error_in(make_srt_error(srt_error_code::srt_handshake_error));
    }

    void srt_socket_service::send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) {
        send(buff, where);
        /// 更新 上一次发送的时间
        last_send_point = clock_type::now();
        /// 发送keepalive
        do_keepalive();
    }
    /// 已经成功建立连接
    void srt_socket_service::on_connect_in() {
        /// 停止计时器
        common_timer->stop();
        _is_connected.store(true, std::memory_order_relaxed);
        /// 清除缓存, 节省内存
        handshake_buffer = nullptr;
        _next_func = std::bind(&srt_socket_service::handle_receive, this, std::placeholders::_1);
        /// 开启keepalive
        do_keepalive();
        /// 记录最后一个包接收的时间
        last_receive_point = clock_type::now();
        common_timer->add_expired_from_now(srt_socket_service::max_receive_time_out, receive_timeout);
        /// 成功连接
        on_connected();
    }

    void srt_socket_service::on_error_in(const std::error_code &e) {
        /// 停止所有定时器
        common_timer->stop();
        keep_alive_timer->stop();
        _is_connected.store(false);
        on_error(e);
    }

    void srt_socket_service::do_keepalive() {
        if (!_is_connected.load(std::memory_order_relaxed)) {
            return;
        }
        /// 一秒一次
        keep_alive_timer->stop();
        keep_alive_timer->add_expired_from_now(1000, keep_alive_expired);
    }

    void srt_socket_service::do_nak() {
    }

    void srt_socket_service::do_ack() {
    }

    void srt_socket_service::do_ack_ack() {
    }

    void srt_socket_service::do_drop_request() {
    }


    void srt_socket_service::do_shutdown() {
        srt_packet pkt;
        pkt.set_control(true);
        pkt.set_control_type(control_type::shutdown);
        pkt.set_timestamp(get_time_from_begin());
        pkt.set_socket_id(srt_socket_service::get_sock_id());
        auto pkt_buffer = create_packet(pkt);
        /// shutdown 直接发送
        send(pkt_buffer, get_remote_endpoint());
    }


    inline uint32_t srt_socket_service::get_time_from_begin() {
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now() - first_connect_point).count());
    }

    void srt_socket_service::handle_reject(int e) {
        auto err = make_srt_reject_error(e);
        on_error_in(err);
    }

    void srt_socket_service::handle_server_induction(const std::shared_ptr<buffer> &buff) {
        Trace("receive server induction..");
        try {
            auto induction_pkt = srt::from_buffer(buff->data(), 16);
            if (induction_pkt->get_control_type() != control_type::handshake) {
                Warn("srt packet control type is not handshake");
                return;
            }

            buff->remove(16);
            auto induction_context = srt::handshake_context::from_buffer(buff->data(), buff->size());
            /// 非法的握手包
            if (induction_context->_req_type != handshake_context::urq_induction) {
                return handle_reject(induction_context->_req_type);
            }
            //// 更新参数
            if (induction_context->_version != 5) {
                /// 握手版本不正确
                return send_reject(handshake_context::packet_type::rej_version, buff);
            }
            /// 版本5的扩展字段检查
            if (induction_context->extension_field != 0x4A17) {
                return send_reject(handshake_context::packet_type::rej_rogue, buff);
            }

            /// 暂不支持加密
            if (induction_context->encryption != 0) {
                return send_reject(handshake_context::packet_type::rej_unsecure, buff);
            }

            /// mtu 检查
            if (induction_context->_max_mss > 1500) {
                return send_reject(handshake_context::packet_type::rej_rogue, buff);
            }
            //// 停止定时器
            common_timer->stop();
            //// 更新包序号
            packet_sequence_number = induction_context->_sequence_number;
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
            ctx._sequence_number = packet_sequence_number;
            ctx._max_mss = srt_socket_base::get_max_payload();
            ctx._window_size = srt_socket_service::get_max_flow_window_size();
            ctx._req_type = srt::handshake_context::urq_conclusion;
            // random
            //// 对应上一个induction的sock id
            ctx._socket_id = get_sock_id();
            ctx._cookie = induction_context->_cookie;
            ctx.address = get_local_endpoint().address();

            auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - first_connect_point).count();
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
        }
    }

    /// conclusion receive
    void srt_socket_service::handle_server_conclusion(const std::shared_ptr<buffer> &buff) {
        Trace("receive server conclusion..");
        try {
            auto size = buff->size();
            auto pkt = srt::from_buffer(buff->data(), buff->size());
            buff->remove(16);
            auto context = srt::handshake_context::from_buffer(buff->data(), buff->size());
            buff->remove(48);
            auto extension = get_extension(*context, buff);

            if (context->_req_type != handshake_context::packet_type::urq_conclusion) {
                return handle_reject(context->_req_type);
            }

            /// 最终更新包序号
            packet_sequence_number = context->_sequence_number;
            /// 最终更新mtu
            /// set_max_payload(context->_max_mss);
            /// 最终更新滑动窗口大小
            auto mss = get_max_payload() > 1500 ? 1500 : get_max_payload();
            context->_max_mss = srt_socket_service::max_payload = mss;
            srt_socket_service::max_flow_window_size = context->_window_size;
            /// 最终确定协商好的参数
            srt_socket_service::sock_id = context->_socket_id;
            srt_socket_service::time_deliver_ = extension->receiver_tlpktd_delay;
            srt_socket_service::drop_too_late_packet = extension->drop;
            srt_socket_service::report_nak = extension->nak;

            Trace("srt handshake success, drop={}, report_nak={}, tsbpd={}, socket_id={}", extension->drop, extension->nak, extension->receiver_tlpktd_delay, get_sock_id());
            return on_connect_in();
        } catch (const std::system_error &e) {
        }
    }


    void srt_socket_service::handle_receive(const std::shared_ptr<buffer> &buff) {
        try {
            auto srt_pkt = from_buffer(buff->data(), buff->size());
            /// 更新上一次收到的时间
            last_receive_point = clock_type::now();
            if (srt_pkt->get_control()) {
                return handle_control(*srt_pkt, buff);
            }
            return handle_data(*srt_pkt, buff);

        } catch (const std::system_error &e) {
        }
    }

    void srt_socket_service::handle_control(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        const auto &type = pkt.get_control_type();
        switch (type) {
            case control_type::handshake:
            case control_type::congestion_warning:
            case control_type::user_defined_type:
                return;
            case control_type::keepalive:
                return handle_keep_alive(pkt, buff);
            case control_type::nak:
                return handle_nak(pkt, buff);
            case control_type::ack:
                return handle_ack(pkt, buff);
            case control_type::ack_ack:
                return handle_ack_ack(pkt, buff);
            case control_type::peer_error:
                return handle_peer_error(pkt, buff);
            case control_type::drop_req:
                return handle_drop_request(pkt, buff);
            case control_type::shutdown:
                return handle_shutdown(pkt, buff);
        }
    }

    void srt_socket_service::handle_data(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        /// 统计数据
        _sock_receive_statistic->report_packet(1);
    }

    void srt_socket_service::handle_keep_alive(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        Info("handle keep alive..");
    }

    void srt_socket_service::handle_nak(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
    }

    void srt_socket_service::handle_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
    }

    void srt_socket_service::handle_ack_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
    }

    void srt_socket_service::handle_peer_error(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        auto e = make_srt_error(srt_error_code::srt_peer_error);
        on_error_in(e);
    }

    void srt_socket_service::handle_drop_request(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
    }

    void srt_socket_service::handle_shutdown(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        std::error_code e = make_srt_error(srt_error_code::peer_has_terminated_connection);
        return on_error_in(e);
    }
};// namespace srt
