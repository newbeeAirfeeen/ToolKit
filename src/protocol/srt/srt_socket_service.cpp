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
        send_rate_limit,
    };

    srt_socket_service::srt_socket_service(asio::io_context &executor) : poller(executor) {
        _sock_send_statistic = std::make_shared<socket_statistic>(poller);
        _sock_receive_statistic = std::make_shared<socket_statistic>(poller);
        _sender_buffer = std::make_shared<sender_queue>(poller);
        _receiver_buffer = std::make_shared<receiver_queue>(poller);
    }

    void srt_socket_service::begin() {
        Trace("初始化公共定时器..");
        common_timer = create_deadline_timer<int>(poller);
        std::weak_ptr<srt_socket_service> self(shared_from_this());
        common_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (stronger_self) {
                stronger_self->on_common_timer_expired(v);
            }
        });
        Trace("初始化keepalive定时器..");
        keep_alive_timer = create_deadline_timer<int>(poller);
        keep_alive_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            stronger_self->on_keep_alive_expired(v);
        });
        Trace("初始化发送队列");
        _sender_buffer->set_sender_output_packet([self](const sender_block_type &b) {
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            return stronger_self->on_sender_packet(b);
        });

        _sender_buffer->set_sender_on_drop_packet([self](size_t begin, size_t end) {
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            return stronger_self->on_sender_drop_packet(begin, end);
        });
        ///
        _sender_buffer->start();
    }

    void srt_socket_service::connect() {
        Trace("srt socket begin connect to {}:{}", get_remote_endpoint().address().to_string(), get_remote_endpoint().port());
        std::default_random_engine random(std::random_device{}());
        std::uniform_int_distribution<int32_t> mt(0, (std::numeric_limits<int32_t>::max)());

        handshake_context ctx;
        ctx._version = 4;
        /// DGRAM
        ctx.extension_field = 2;
        /// random seq
        ctx._sequence_number = static_cast<uint32_t>(mt(random));
        ctx._max_mss = srt_socket_base::get_max_payload();
        ctx._window_size = srt_socket_service::get_max_flow_window_size();
        ctx._req_type = srt::handshake_context::urq_induction;
        // random
        ctx._socket_id = static_cast<uint32_t>(mt(random));
        ctx._cookie = static_cast<uint32_t>(mt(random));
        ctx.address = get_local_endpoint().address();


        srt_packet pkt;
        pkt.set_control_type(control_type::handshake);
        /// srt_packet
        auto _pkt = create_packet(pkt);
        handshake_context::to_buffer(ctx, _pkt);
        /// save induction message
        handshake_buffer = _pkt;
        connect_point = clock_type::now();
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
                auto ts = get_time_from<std::chrono::milliseconds>(connect_point);
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

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - last_send_point).count();
        if (now < 1000) {
            return keep_alive_timer->add_expired_from_now(1000 - now, keep_alive_expired);
        }

        auto ts = get_time_from<std::chrono::microseconds>(connect_point);
        if (!keep_alive_buffer) {
            srt_packet pkt;
            pkt.set_control_type(control_type::keepalive);
            pkt.set_socket_id(srt_socket_base::get_sock_id());
            pkt.set_timestamp(ts);
            keep_alive_buffer = create_packet(pkt);
        } else {
            /// 更新时间戳
            set_packet_timestamp(keep_alive_buffer, ts);
        }
        send_in(keep_alive_buffer, get_remote_endpoint());
        return keep_alive_timer->add_expired_from_now(1000, keep_alive_expired);
    }

    inline uint32_t srt_socket_service::get_next_packet_message_number() {
        auto tmp = message_number;
        message_number = (message_number + 1) % message_max_seq;
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

    int srt_socket_service::async_send(const std::shared_ptr<buffer> &buff) {

        if (buff->size() >= 1500) {
            throw std::system_error(make_srt_error(srt_error_code::too_large_payload));
        }

        if (!_sender_buffer->capacity()) {
            return -1;
        }

        std::weak_ptr<srt_socket_service> self(shared_from_this());
        auto block = std::make_shared<typename sender_queue::block>();
        poller.post([self, buff, block]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }

            if (!stronger_self->_is_connected.load(std::memory_order_relaxed)) {
                auto e = make_srt_error(srt_error_code::not_connected_yet);
                /// 在连接的时候 直接调用.不终止会话loop
                return stronger_self->on_error(e);
            }

            /// 构造srt packet
            srt_packet pkt;
            pkt.set_control(false);
            pkt.set_packet_sequence_number(stronger_self->_sender_buffer->get_initial_sequence());
            pkt.set_message_number(stronger_self->get_next_packet_message_number());
            pkt.set_timestamp(stronger_self->get_time_from<std::chrono::microseconds>(stronger_self->connect_point));
            pkt.set_socket_id(stronger_self->srt_socket_service::get_sock_id());
            auto pkt_buf = create_packet(pkt);
            pkt_buf->append(buff->data(), buff->size());
            block->content = pkt_buf;
            /// 放入发送缓冲
            return stronger_self->_sender_buffer->insert(block);
        });
        return 0;
    }

    void srt_socket_service::on_sender_packet(const sender_block_type &type) {
        auto _type = type;
        if (!_type) {
            return;
        }

        auto content = _type->content;
        if (!content) {
            return;
        }
        //// 发送数据
        if (_type->is_retransmit) {
            set_retransmit(true, content);
        }
        return send_in(content, get_remote_endpoint());
    }
    /// 发送缓冲区 主动丢包回调
    void srt_socket_service::on_sender_drop_packet(size_t begin, size_t end) {
        Info("drop packet {}-{}", begin, end);
        /// 需要发送 message drop request
        return do_drop_request(begin, end);
    }


    void srt_socket_service::send_reject(int e, const std::shared_ptr<buffer> &buff) {
        Trace("send_reject..");
        if (buff->size() < 40) {
            throw std::invalid_argument("buff is MUST greater than 40");
        }
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
        Warn("invoke connected callback end");
    }

    void srt_socket_service::on_error_in(const std::error_code &e) {
        /// 停止发送队列
        _sender_buffer->stop();
        /// 停止相关定时器
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
        keep_alive_timer->add_expired_from_now(1000, keep_alive_expired);
    }

    void srt_socket_service::do_nak() {
    }

    void srt_socket_service::do_ack() {
    }

    void srt_socket_service::do_ack_ack(uint32_t ack_number) {
        Trace("send ack ack {}", ack_number);
        srt_packet pkt;
        pkt.set_control_type(ack_ack);
        pkt.set_type_information(ack_number);
        pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        pkt.set_socket_id(get_sock_id());
        auto ack_ack_buff = create_packet(pkt);
        send_in(ack_ack_buff, get_remote_endpoint());
    }


    /// a message drop request control packet is sent by the sender to the
    /// receiver when a retransmission of an unacknowledged packet
    void srt_socket_service::do_drop_request(size_t begin, size_t end) {
        Trace("send drop request {} to {}", begin, end);
        srt_packet pkt;
        pkt.set_control_type(control_type::drop_req);
        pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        auto drop_buf = create_packet(pkt);
        if (begin != end) {
            begin = begin | 0x80000000;
        }
        drop_buf->put_be<uint32_t>(begin);
        if (begin != end) {
            drop_buf->put_be<uint32_t>(end);
        }

        return send_in(drop_buf, get_remote_endpoint());
    }


    void srt_socket_service::do_shutdown() {
        srt_packet pkt;
        pkt.set_control_type(control_type::shutdown);
        pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        pkt.set_socket_id(srt_socket_service::get_sock_id());
        auto pkt_buffer = create_packet(pkt);
        /// shutdown 直接发送
        send(pkt_buffer, get_remote_endpoint());
        on_error_in(make_srt_error(srt_error_code::socket_shutdown_op));
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
            /// 更新mtu
            set_max_payload(induction_context->_max_mss);
            /// 更新滑动窗口大小
            set_max_flow_window_size(induction_context->_window_size);
            /// 更新socket_id
            set_sock_id(induction_context->_socket_id);

            handshake_context ctx;
            ctx._version = 5;
            ctx._sequence_number = induction_context->_sequence_number;

            ctx._max_mss = srt_socket_base::get_max_payload();
            ctx._window_size = srt_socket_service::get_max_flow_window_size();
            ctx._req_type = srt::handshake_context::urq_conclusion;
            // random
            //// 对应上一个induction的sock id
            ctx._socket_id = get_sock_id();
            ctx._cookie = induction_context->_cookie;
            ctx.address = get_local_endpoint().address();

            auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - connect_point).count();
            srt_packet pkt;
            pkt.set_control_type(control_type::handshake);
            pkt.set_timestamp(static_cast<uint32_t>(ts));
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
            //// 如果允许丢包
            if (srt_socket_service::drop_too_late_packet && srt_socket_service::time_deliver_) {
                _sender_buffer->set_max_delay(srt_socket_service::time_deliver_ < 120 ? 120 : srt_socket_service::time_deliver_);
            }
            /// 最终更新发送包序号
            _sender_buffer->clear();
            _sender_buffer->set_initial_sequence(context->_sequence_number);
            _sender_buffer->set_window_size(context->_window_size);
            _sender_buffer->set_max_sequence(packet_max_seq);
            Trace("srt handshake success, initial sequence={}, drop={}, report_nak={}, tsbpd={}, socket_id={}, window_size={}", context->_sequence_number, extension->drop, extension->nak,
                  extension->receiver_tlpktd_delay, get_sock_id(), context->_window_size);
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
        buff->remove(16);
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
    /// Negative acknowledgment control packets are used to signal failed data packet deliveries
    /// The receiver notifies the sender about lost data packets by sending a NAK packet that contains
    /// a list of sequence numbers for those lost packets.
    /// Appendix A Packet Sequence List Coding
    /// For any single packet sequence number, it uses the origin sequence number in the field.
    /// The first bit MUST start with "0"
    ///
    /// For any consecutive packet sequence numbers that the difference between the last and first
    /// is more than 1, only record the first and the last sequence in the list field, and modify
    /// the first bit of to "1"
    void srt_socket_service::handle_nak(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        while (buff->size() >= 4) {
            uint32_t seq_begin = buff->get_be<uint32_t>();
            /// 说明是范围包
            if (seq_begin & 0x80000000 && buff->size() >= 4) {
                uint32_t seq_end = buff->get_be<uint32_t>();
                seq_begin = seq_begin & 0x7FFFFFFF;
                ///seq_end 之前的全部重新发送一遍
                Trace("handle nak {}-{}", seq_begin, seq_end);
                _sender_buffer->try_send_again(seq_begin, seq_end);
                return;
            }
            //// 说明是单个包
            _sender_buffer->try_send_again(seq_begin, seq_begin);
        }
    }

    /// ack control packet are used to provide the delivery
    void srt_socket_service::handle_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        auto an = pkt.get_type_information();
        /// a light ack control packet includes only the last Acknowledged Packets sequence number field
        /// the type-specific information field should be set to 0.
        if (buff->size() < 4) {
            return;
        }

        uint32_t _last_ack_packet_seq = buff->get_be<uint32_t>();
        Trace("ack number:{}, last_ack_packet_seq={}", an, _last_ack_packet_seq);
        if (buff->size() >= 12) {
            /// A Small ACK includes the fields up to and including the Available
            /// Buffer Size field
            uint32_t _rtt = buff->get_be<uint32_t>();
            uint32_t _rtt_variance = buff->get_be<uint32_t>();
            uint32_t _available_buffer_size = buff->get_be<uint32_t>();
            if (buff->size() >= 12) {
                uint32_t _packet_receiving_rate = buff->get_be<uint32_t>();
                uint32_t _estimated_link_capacity = buff->get_be<uint32_t>();
                uint32_t _receiving_rate = buff->get_be<uint32_t>();
                /// ack ack control packets are sent to acknowledge the reception
                do_ack_ack(an);
            }
        }
        /// 滑动序号
        ///Info("before sequence, {}-{}", _sender_buffer->get_first_block()->sequence_number, _sender_buffer->get_last_block()->sequence_number);
        _sender_buffer->sequence_to(_last_ack_packet_seq);

    }

    void srt_socket_service::handle_ack_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
    }

    void srt_socket_service::handle_peer_error(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        auto e = make_srt_error(srt_error_code::srt_peer_error);
        on_error_in(e);
    }

    void srt_socket_service::handle_drop_request(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        if (buff->size() < 8) {
            return;
        }
        uint32_t first_packet_seq = buff->get_be<uint32_t>();
        uint32_t last_packet_seq = buff->get_be<uint32_t>();
    }

    void srt_socket_service::handle_shutdown(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        std::error_code e = make_srt_error(srt_error_code::peer_has_terminated_connection);
        return on_error_in(e);
    }
};// namespace srt
