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
#include "packet_limited_send_rate_queue.hpp"
#include "packet_receive_queue.hpp"
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
    /// 连接回调, 发生在握手
    enum timer_expired_type {
        keep_alive_expired,
        induction_expired,
        conclusion_expired,
        receive_timeout,
        send_rate_limit,
        server_handshake,
        nak_expired,
        ack_expired,
    };

    srt_socket_service::srt_socket_service(asio::io_context &executor) : poller(executor) {
        _ack_queue_ = std::make_shared<srt::srt_ack_queue>();
    }

    void srt_socket_service::begin() {
        Debug("init common timer..");
        common_timer = create_deadline_timer<int>(poller);
        std::weak_ptr<srt_socket_service> self(shared_from_this());
        common_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (stronger_self) {
                stronger_self->on_common_timer_expired(v);
            }
        });
        Trace("init keep alive timer...");
        keep_alive_timer = create_deadline_timer<int>(poller);
        keep_alive_timer->set_on_expired([self](const int &v) {
            auto stronger_self = self.lock();
            if (!stronger_self)
                return;
            stronger_self->on_keep_alive_expired(v);
        });
    }

    uint32_t srt_socket_service::get_cookie() {
        return 0;
    }

    asio::io_context &srt_socket_service::get_poller() {
        return this->poller;
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
        ctx.address = get_local_endpoint().address();


        srt_packet pkt;
        pkt.set_control_type(control_type::handshake);
        /// srt_packet
        Trace("srt induction handshake version={}, seq={}, mss={}, window_size={}", ctx._version, ctx._sequence_number, ctx._max_mss, ctx._window_size);
        auto _pkt = create_packet(pkt);
        handshake_context::to_buffer(ctx, _pkt);
        /// save induction message
        handshake_buffer = _pkt;
        connect_point = std::chrono::steady_clock::now();
        {
            _next_func = std::bind(&srt_socket_service::handle_server_induction, this, std::placeholders::_1);
            _next_func_with_pkt = std::bind(&srt_socket_service::handle_server_induction_1, this, std::placeholders::_1, std::placeholders::_2);
        }
        /// 发送到对端
        send_in(handshake_buffer, get_remote_endpoint());
        /// 开始定时器, 每隔一段时间发送induction包
        Trace("update timer after 250ms to send induction again");
        this->common_timer->add_expired_from_now(250, timer_expired_type::induction_expired);
        /// 套接字已经打开
        _is_open.store(true, std::memory_order_relaxed);
    }

    void srt_socket_service::connect_as_server() {
        _next_func = std::bind(&srt_socket_service::handle_client_induction, this, std::placeholders::_1);
        _next_func_with_pkt = std::bind(&srt_socket_service::handle_client_induction_1, this, std::placeholders::_1, std::placeholders::_2);
        connect_point = std::chrono::steady_clock::now();
    }

    void srt_socket_service::on_common_timer_expired(const int &val) {
        int v = val;
        if (perform_error) return;
        switch (v) {
            /// 用于客户端握手
            case timer_expired_type::induction_expired:
            case timer_expired_type::conclusion_expired: {
                Trace("connect package may be lost, try send again, package type={}", v);
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
                /// 250ms后尝试重新发送
                Trace("update timer after 250ms to send induction again");
                this->common_timer->add_expired_from_now(250, val);
                break;
            }
            /// 接收超时 断开连接
            case timer_expired_type::receive_timeout: {
                /// 上一次接收到到时间点
                uint32_t leave_last_receive_time_point = get_time_from<std::chrono::milliseconds>(last_receive_point);
                if (leave_last_receive_time_point > srt_socket_service::max_receive_time_out) {
                    do_shutdown();
                } else {
                    common_timer->add_expired_from_now(srt_socket_service::max_receive_time_out - leave_last_receive_time_point, timer_expired_type::receive_timeout);
                }
                break;
            }
            /// 服务端握手超时
            case timer_expired_type::server_handshake: {
                Error("client handshake time out...");
                srt_packet pkt;
                return send_reject(1007, handshake_buffer);
            }
            case timer_expired_type::nak_expired: {
                return do_nak_in();
            }
            case timer_expired_type::ack_expired: {
                return do_ack_in();
            }
        }
    }

    void srt_socket_service::on_keep_alive_expired(const int &v) {
        /// 如果没有连接 停止发送
        if (!_is_connected.load(std::memory_order_relaxed) || perform_error) {
            return;
        }

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_send_point).count();
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

    void srt_socket_service::input_packet(const std::shared_ptr<buffer> &buff) {
        _next_func(buff);
    }

    void srt_socket_service::input_packet(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        _next_func_with_pkt(pkt, buff);
    }

    bool srt_socket_service::is_open() {
        return _is_open.load(std::memory_order_relaxed);
    }

    bool srt_socket_service::is_connected() {
        return _is_connected.load(std::memory_order_relaxed);
    }

    int srt_socket_service::async_send(const char *data, size_t length) {
        auto mss = get_max_payload();
        size_t write_bytes = length;
        size_t offset = 0;
        int ret = 0;
        while (write_bytes > 0) {
            /// 最后一个包
            if (write_bytes <= mss) {
                ret = async_send(std::make_shared<buffer>(data + offset, write_bytes));
                if (ret > 0) {
                    write_bytes -= ret;
                }
                break;
            }
            ret = async_send(std::make_shared<buffer>(data + offset, write_bytes));
            if (ret == -1 || ret == 0) {
                break;
            }
            /// 移动
            offset += mss;
            write_bytes -= mss;
        }
        return ret == -1 ? -1 : static_cast<int>(length - write_bytes);
    }

    int srt_socket_service::async_send(const std::shared_ptr<buffer> &buff) {
        if (buff->size() > get_max_payload()) {
            throw std::system_error(make_srt_error(srt_error_code::too_large_payload));
        }

        if (!_is_connected.load(std::memory_order_relaxed)) {
            auto e = make_srt_error(srt_error_code::not_connected_yet);
            /// 在连接的时候 直接调用.不终止会话loop
            on_error(e);
            return -1;
        }

        return _sender_queue->input_packet(buff, 0, 0);
    }

    void srt_socket_service::on_sender_packet(const packet_pointer &type) {
        if (perform_error || !type) {
            return;
        }
        if (type->retransmit_count > 1) {
            set_retransmit(true, type->pkt);
        }
        return send_in(type->pkt, get_remote_endpoint());
    }
    /// 发送缓冲区 主动丢包回调
    void srt_socket_service::on_sender_drop_packet(size_t begin, size_t end) {
        if (perform_error) {
            return;
        }
        Info("drop packet {}-{}", begin, end);
        /// 需要发送 message drop request
        return do_drop_request(begin, end);
    }


    void srt_socket_service::on_receive_packet(const packet_pointer &type) {
        if (perform_error) {
            return;
        }
        return onRecv(type->pkt);
    }

    void srt_socket_service::on_receive_drop_packet(size_t begin, size_t end) {
        Warn("active drop packet {}-{}", begin, end);
    }

    void srt_socket_service::send_reject(int e, const std::shared_ptr<buffer> &buff) {
        Trace("send_reject, error_code={}", e);
        if (buff->size() < 40) {
            throw std::invalid_argument("buff is MUST greater than 40");
        }
        ////
        buff->backward();
        auto *p = (uint32_t *) (buff->data() + 36);
        /// conclusion error code
        set_be32(p, static_cast<uint32_t>(e));
        auto reject_buf = std::make_shared<buffer>(buff->data(), buff->size());
        send_in(reject_buf, get_remote_endpoint());
        /// 最后调用on_error
        on_error_in(make_srt_error(srt_error_code::srt_handshake_error));
    }

    void srt_socket_service::send_in(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) {
        send(buff, where);
        /// 更新 上一次发送的时间
        last_send_point = std::chrono::steady_clock::now();
    }


    /// 已经成功建立连接
    void srt_socket_service::on_connect_in() {
        /// 记录最后一个包接收的时间
        last_receive_point = std::chrono::steady_clock::now();
        connect_point = last_receive_point;
        if (get_max_payload() >= 1500)
            srt_socket_service::max_payload = 1456;

        Trace("init sender/receiver buffer queue...");
        _sender_queue = std::make_shared<packet_limited_send_rate_queue<std::shared_ptr<buffer>>>(poller, _ack_queue_, get_report_nak(),
                                                                                                  get_sock_id(), connect_point, get_max_payload());
        _receive_queue = std::make_shared<packet_receive_queue<std::shared_ptr<buffer>>>();

        _sender_queue->set_on_packet(std::bind(&srt_socket_service::on_sender_packet, this, std::placeholders::_1));
        _sender_queue->set_on_drop_packet(std::bind(&srt_socket_service::on_sender_drop_packet, this, std::placeholders::_1, std::placeholders::_2));
        _receive_queue->set_on_packet(std::bind(&srt_socket_service::on_receive_packet, this, std::placeholders::_1));
        _receive_queue->set_on_drop_packet(std::bind(&srt_socket_service::on_receive_drop_packet, this, std::placeholders::_1, std::placeholders::_2));

        _sender_queue->set_current_sequence(_handshake_context->_sequence_number);
        _sender_queue->set_max_sequence(packet_max_seq);
        _sender_queue->set_window_size(_handshake_context->_window_size);
        _sender_queue->update_flow_window(_handshake_context->_window_size);
        _sender_queue->ack_sequence_to(_handshake_context->_sequence_number);

        _receive_queue->set_current_sequence(_handshake_context->_sequence_number);
        _receive_queue->set_max_sequence(packet_max_seq);
        _receive_queue->set_window_size(_handshake_context->_window_size);
        //// 如果允许丢包
        if (srt_socket_service::drop_too_late_packet && srt_socket_service::time_deliver_) {
            auto delay = srt_socket_service::time_deliver_ < 120 ? 120 : srt_socket_service::time_deliver_;
            _sender_queue->set_max_delay(delay < 1020 ? 1020 : delay);
            _receive_queue->set_max_delay(delay < 1020 ? 1020 : delay);
        }

        Trace("start sender/receiver buffer queue...");
        _sender_queue->start();
        _receive_queue->start();
        /// 停止计时器
        common_timer->stop();
        _is_open.store(true, std::memory_order_relaxed);
        _is_connected.store(true, std::memory_order_relaxed);
        {
            _next_func = std::bind(&srt_socket_service::handle_receive, this, std::placeholders::_1);
            _next_func_with_pkt = std::bind(&srt_socket_service::handle_receive_1, this, std::placeholders::_1, std::placeholders::_2);
        }
        /// 开启keepalive
        do_keepalive();
        /// 创建
        _packet_receive_rate_ = std::make_shared<packet_calculate_window<16, 64>>();
        common_timer->add_expired_from_now(srt_socket_service::max_receive_time_out, receive_timeout);
        /// 成功连接
        on_connected();
        Trace("invoke connected callback end");
    }

    void srt_socket_service::on_error_in(const std::error_code &e) {
        /// 停止发送队列
        /// 停止相关定时器
        common_timer->stop();
        keep_alive_timer->stop();
        _is_connected.store(false);
        if (_sender_queue) _sender_queue->clear();
        if (_receive_queue) _receive_queue->clear();
        if (!perform_error) {
            perform_error = true;
            Trace("there is something error, perform_error={}", perform_error);
            on_error(e);
        }
    }

    void srt_socket_service::do_keepalive() {
        /// 一秒一次
        keep_alive_timer->add_expired_from_now(1000, keep_alive_expired);
    }

    /// Negative acknowledgment (NAK) control packets are used to signal
    /// failed data packet deliveries. The receiver notifies the sender
    /// about lost data packets by sending a NAK packet that contains a list
    /// of sequence numbers for those lost packets.
    void srt_socket_service::do_nak() {
        report_nak_begin = true;
        return do_nak_in();
    }

    void srt_socket_service::do_nak_in() {
        //// 检查接收队列
        auto vec = _receive_queue->get_pending_packets();
        if (vec.empty()) {
            report_nak_begin = false;
            return;
        }
        auto buff = std::make_shared<buffer>();
        buff->reserve(vec.size() * 8);
        for (const auto &item: vec) {
            auto first = item.first;
            auto second = item.second;
            if (first != second) {
                buff->put_be<uint32_t>(0x80000000 | first);
                buff->put_be<uint32_t>(second);
            } else {
                buff->put_be<uint32_t>(first);
            }
        }
        srt_packet pkt;
        pkt.set_control_type(nak);
        pkt.set_socket_id(get_sock_id());
        pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        auto pkt_buff = create_packet(pkt);
        pkt_buff->append(buff->data(), buff->size());
        Debug("send nak, pair size={}", vec.size());
        send_in(pkt_buff, get_remote_endpoint());
        uint32_t nak_interval = (_ack_queue_->get_rto() + 4 * _ack_queue_->get_rtt_var()) / 2;
        nak_interval = nak_interval < 20 ? 20 : nak_interval;
        common_timer->add_expired_from_now(nak_interval, nak_expired);
    }

    void srt_socket_service::do_ack() {
        ack_begin = true;
        last_ack_response = std::chrono::steady_clock::now();
        /// 至少执行一次
        common_timer->add_expired_from_now(10, ack_expired);
    }

    void srt_socket_service::do_ack_in() {
        auto buff = std::make_shared<buffer>();
        auto seq = _receive_queue->get_current_sequence();
        auto rto = _ack_queue_->get_rto();
        auto rtt_var = _ack_queue_->get_rtt_var();
        auto now = std::chrono::steady_clock::now();
        auto receive_rate = _packet_receive_rate_->get_pkt_receive_rate();
        buff->reserve(28);
        /// last acknowledged packet seq
        buff->put_be<uint32_t>(seq);
        /// rtt
        buff->put_be<uint32_t>(rto);
        /// rtt variance
        buff->put_be<uint32_t>(rtt_var);
        /// capacity
        buff->put_be<uint32_t>(_receive_queue->capacity());
        /// packet receive rate
        buff->put_be<uint32_t>(receive_rate.pkt_per_sec);
        /// estimated link capacity
        buff->put_be<uint32_t>(_packet_receive_rate_->get_bandwidth());
        /// receiving rate
        buff->put_be<uint32_t>(receive_rate.bytes_per_sec);
        srt_packet pkt;
        pkt.set_control_type(ack);
        pkt.set_type_information(ack_number);
        pkt.set_timestamp((uint32_t) std::chrono::duration_cast<std::chrono::microseconds>(now - connect_point).count());
        pkt.set_socket_id(get_sock_id());
        /// 添加到ack队列中
        _ack_queue_->add_ack(ack_number);
        ack_number = (ack_number + 1) % 0xFFFFFFFF;
        auto pkt_buff = create_packet(pkt);
        pkt_buff->append(buff->data(), buff->size());
        send_in(pkt_buff, get_remote_endpoint());

        if (seq != std::get<0>(_ack_entry)) {
            _ack_entry = std::make_tuple(seq, now);
        }
        auto RTO = 3 * (rto + 4 * rtt_var + 20000) + 10000;
        auto spend = std::chrono::duration_cast<std::chrono::microseconds>(now - std::get<1>(_ack_entry)).count();
        if (spend > RTO) {
            Trace("stop to receive data, time out of RTO, RTO={} us, spend={} us", RTO, spend);
            _receive_queue->clear();
            ack_begin = false;
            return;
        }
        common_timer->add_expired_from_now(10, ack_expired);
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
        pkt.set_socket_id(get_sock_id());
        pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        auto drop_buf = create_packet(pkt);
        drop_buf->put_be<uint32_t>(static_cast<uint32_t>(begin));
        drop_buf->put_be<uint32_t>(static_cast<uint32_t>(end));
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
            return handle_server_induction_1(induction_pkt, buff);
        } catch (const std::system_error &e) {
            Error("catch exception, code={}, msg={}", e.code().value(), e.what());
        }
    }

    void srt_socket_service::handle_server_induction_1(const std::shared_ptr<srt_packet> &induction_pkt, const std::shared_ptr<buffer> &buff) {
        if (induction_pkt->get_control_type() != control_type::handshake) {
            Warn("srt packet control type is not handshake");
            return;
        }

        buff->remove(16);
        auto induction_context = srt::handshake_context::from_buffer(buff->data(), buff->size());
        /// 非法的握手包
        if (induction_context->_req_type != handshake_context::urq_induction) {
            Debug("invalid handshake packet, not a induction packet");
            return handle_reject(induction_context->_req_type);
        }
        //// 更新参数
        if (induction_context->_version != 5) {
            /// 握手版本不正确
            Debug("srt version is not equal to 5, which is forbidden");
            return send_reject(handshake_context::packet_type::rej_version, buff);
        }
        /// 版本5的扩展字段检查
        if (induction_context->extension_field != 0x4A17) {
            Debug("srt extension field is not 0x4A17");
            return send_reject(handshake_context::packet_type::rej_rogue, buff);
        }

        /// 暂不支持加密
        if (induction_context->encryption != 0) {
            Debug("current is not support encryption");
            return send_reject(handshake_context::packet_type::rej_unsecure, buff);
        }

        /// mtu 检查
        if (induction_context->_max_mss > 1500) {
            Debug("max payload is not exceed 1500");
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
        ////
        ctx.extension_field = 1;
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
        set_extension(ctx, _pkt, get_time_based_deliver(), get_drop_too_late_packet(), get_report_nak(), 0, get_stream_id());
        /// 最后更新extension_field
        handshake_context::update_extension_field(ctx, _pkt);
        Trace("send conclusion handshake, version={}, seq={}, mtu={}, window_size={}, sock_id={}, cookie={}", ctx._version, ctx._sequence_number,
              ctx._max_mss, ctx._window_size, ctx._socket_id, ctx._cookie);
        /// 更新握手上下文
        {
            _next_func = std::bind(&srt_socket_service::handle_server_conclusion, this, std::placeholders::_1);
            _next_func_with_pkt = std::bind(&srt_socket_service::handle_server_conclusion_1, this, std::placeholders::_1, std::placeholders::_2);
        }
        /// 保存握手缓存
        handshake_buffer = _pkt;
        send_in(_pkt, get_remote_endpoint());
        common_timer->add_expired_from_now(250, conclusion_expired);
    }
    /// conclusion receive
    void srt_socket_service::handle_server_conclusion(const std::shared_ptr<buffer> &buff) {
        Trace("receive server conclusion..");
        try {
            auto size = buff->size();
            auto pkt = srt::from_buffer(buff->data(), buff->size());
            buff->remove(16);
            return handle_server_conclusion_1(pkt, buff);
        } catch (const std::system_error &e) {
            Error("catch exception, code={}, msg={}", e.code().value(), e.what());
        }
    }

    void srt_socket_service::handle_server_conclusion_1(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        auto context = srt::handshake_context::from_buffer(buff->data(), buff->size());
        buff->remove(48);
        auto extension = get_extension(*context, buff);

        if (context->_req_type != handshake_context::packet_type::urq_conclusion) {
            Debug("server response is not urq conclusion");
            return handle_reject(context->_req_type);
        }

        if (extension->buffer_mode) {
            Debug("server handshake in buffer mode, reject it");
            return send_reject(1012, buff);
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
        _handshake_context = context;
        Trace("srt handshake success, initial sequence={}, drop={}, report_nak={}, tsbpd={}, socket_id={}, window_size={}", context->_sequence_number, extension->drop, extension->nak,
              extension->receiver_tlpktd_delay, get_sock_id(), context->_window_size);
        return on_connect_in();
    }

    void srt_socket_service::handle_client_induction(const std::shared_ptr<buffer> &buff) {
        try {
            auto pkt = from_buffer(buff->data(), buff->size());
            buff->remove(16);
            return handle_client_induction_1(pkt, buff);
        } catch (const std::system_error &e) {
            Error("catch exception, code={}, msg={}", e.code().value(), e.what());
        }
    }

    void srt_socket_service::handle_client_induction_1(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        try {
            /// 首先解析握手上下文
            auto _tmp_context = handshake_context::from_buffer(buff->data(), buff->size());
            buff->remove(48);
            if (_tmp_context->_req_type == handshake_context::packet_type::urq_conclusion) {
                if (handshake_conclusion) {
                    return handle_client_conclusion(pkt, _tmp_context, buff);
                }
                Warn("invalid conclusion before induction, ignore it");
                return;
            }
            /// 保存上下文
            _handshake_context = _tmp_context;
            /// 保存上下文
            _handshake_context->_version = 5;
            if (_handshake_context->encryption) {
                Error("not support secret, send reject to peer");
                return send_reject(1011, buff);
            }

            std::default_random_engine random(std::random_device{}());
            std::uniform_int_distribution<int32_t> mt(0, (std::numeric_limits<int32_t>::max)());
            _handshake_context->extension_field = 0x4A17;
            _handshake_context->_max_mss = _handshake_context->_max_mss > 1500 ? 1500 : _handshake_context->_max_mss;
            _handshake_context->_window_size = _handshake_context->_window_size < 8192 ? 8192 : _handshake_context->_window_size;
            _handshake_context->_req_type = handshake_context::packet_type::urq_induction;
            _handshake_context->address = get_local_endpoint().address();
            _handshake_context->_cookie = get_cookie();
            handshake_conclusion = 1;
            srt_packet pkt;
            pkt.set_socket_id(_handshake_context->_socket_id);
            pkt.set_control_type(handshake);
            pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
            auto pkt_buffer = create_packet(pkt);
            handshake_context::to_buffer(*_handshake_context, pkt_buffer);
            /// 保存握手缓存
            handshake_buffer = pkt_buffer;
            /// 发送induction
            send_in(pkt_buffer, get_remote_endpoint());
            /// 设置服务端握手超时
            common_timer->add_expired_from_now(srt_socket_base::get_connect_timeout(), server_handshake);
        } catch (const std::system_error &e) {
            Error(e.code().message());
        }
    }

    void srt_socket_service::handle_client_conclusion(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<handshake_context> &context, const std::shared_ptr<buffer> &buff) {
        if (handshake_conclusion == 2) {
            /// 表示已经握手成功
            /// 直接发送缓存
            return send_in(handshake_buffer, get_remote_endpoint());
        }

        std::shared_ptr<extension_field> extension;
        try {
            if (buff->size() <= 0) {
                Error("no extension field in version 5");
                throw std::runtime_error("");
            }
            extension = get_extension(*context, buff);
        } catch (...) {
            return send_reject(1003, buff);
        }

        if (extension->buffer_mode) {
            Error("client handshake with buffer mode, reject it");
            return send_reject(1012, buff);
        }

        if (context->_version != 5) {
            Error("client version at least 5, send reject to peer");
            return send_reject(1008, buff);
        }
        if (context->encryption != 0) {
            Error("not support encryption");
            return send_reject(1011, buff);
        }
        if (context->extension_field != 1) {
            Error("stream type is not equal to 1");
            return send_reject(1003, buff);
        }
        if (context->_max_mss > 1500 || context->_max_mss < 728) {
            Error("invalid mss size={}", context->_max_mss);
            return send_reject(1003, buff);
        }
        if (context->_window_size < 8192) {
            Error("invalid window size ={}", context->_window_size);
            return send_reject(1003, buff);
        }

        std::default_random_engine random(std::random_device{}());
        std::uniform_int_distribution<int32_t> mt(0, (std::numeric_limits<int32_t>::max)());
        _handshake_context = context;
        _handshake_context->_socket_id = mt(random);
        _handshake_context->address = get_local_endpoint().address();
        srt_socket_base::set_max_payload(_handshake_context->_max_mss);
        srt_socket_base::set_max_flow_window_size(_handshake_context->_window_size);
        srt_socket_base::set_sock_id(_handshake_context->_socket_id);

        srt_socket_base::set_report_nak(extension->nak);
        srt_socket_base::set_drop_too_late_packet(extension->drop);
        srt_socket_base::set_time_based_deliver(extension->receiver_tlpktd_delay);
        srt_socket_base::set_stream_id(extension->stream_id);
        Trace("client handshake success, max_mss={}, window size={}, sock_id={}, report_nak={}, enable_drop={}, time based={} ms, stream_id={}",
              _handshake_context->_max_mss, _handshake_context->_window_size, _handshake_context->_socket_id,
              extension->nak, extension->drop, extension->receiver_tlpktd_delay, extension->stream_id);
        handshake_conclusion = 2;
        srt_packet _pkt;
        _pkt.set_control_type(handshake);
        _pkt.set_timestamp(get_time_from<std::chrono::microseconds>(connect_point));
        handshake_buffer = create_packet(_pkt);
        handshake_context::to_buffer(*_handshake_context, handshake_buffer);
        set_extension(*_handshake_context, handshake_buffer, extension->receiver_tlpktd_delay, extension->drop, extension->nak, extension->receiver_tlpktd_delay);
        if (extension->drop) {
            auto delay = extension->receiver_tlpktd_delay < 120 ? 120 : extension->receiver_tlpktd_delay;
            set_time_based_deliver(delay);
        }
        send_in(handshake_buffer, get_remote_endpoint());
        on_connect_in();
    }

    void srt_socket_service::handle_receive(const std::shared_ptr<buffer> &buff) {
        try {
            auto srt_pkt = from_buffer(buff->data(), buff->size());
            buff->remove(16);
            return handle_receive_1(srt_pkt, buff);

        } catch (const std::system_error &e) {
            Error("catch exception, code={}, msg={}", e.code().value(), e.what());
        }
    }

    void srt_socket_service::handle_receive_1(const std::shared_ptr<srt_packet> &srt_pkt, const std::shared_ptr<buffer> &buff) {
        if (perform_error) {
            return;
        }

        if (srt_pkt->get_socket_id() != srt_socket_base::sock_id) {
            Warn("receive socket id is not equal to current socket id which is invalid, ignore it");
            return;
        }

        /// 更新上一次收到的时间
        last_receive_point = std::chrono::steady_clock::now();
        if (_packet_receive_rate_)
            _packet_receive_rate_->update_receive_rate((uint16_t) buff->size() + 16);

        if (srt_pkt->get_control()) {
            return handle_control(srt_pkt, buff);
        }
        return handle_data(srt_pkt, buff);
    }

    void srt_socket_service::handle_control(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        const auto &type = pkt->get_control_type();
        switch (type) {
            case control_type::handshake:
                return handle_client_induction_1(pkt, buff);
            case control_type::congestion_warning:
            case control_type::user_defined_type:
                return;
            case control_type::keepalive:
                return handle_keep_alive(*pkt, buff);
            case control_type::nak:
                return handle_nak(*pkt, buff);
            case control_type::ack:
                return handle_ack(*pkt, buff);
            case control_type::ack_ack:
                return handle_ack_ack(*pkt, buff);
            case control_type::peer_error:
                return handle_peer_error(*pkt, buff);
            case control_type::drop_req:
                return handle_drop_request(*pkt, buff);
            case control_type::shutdown:
                return handle_shutdown(*pkt, buff);
        }
    }

    void srt_socket_service::handle_data(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        /// 统计数据
        /// 丢入接收队列
        if (_packet_receive_rate_)
            _packet_receive_rate_->update_estimated_capacity(pkt->get_packet_sequence_number(), (uint16_t) buff->size() + 16, pkt->get_in_order() || pkt->is_retransmitted());
        _receive_queue->input_packet(buff, pkt->get_packet_sequence_number(), pkt->get_time_stamp());
        if (srt_socket_base::report_nak && !report_nak_begin) {
            do_nak();
        }
        if (!ack_begin) {
            do_ack();
        }
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
            if ((seq_begin & 0x80000000) && buff->size() >= 4) {
                uint32_t seq_end = buff->get_be<uint32_t>();
                seq_begin = seq_begin & 0x7FFFFFFF;
                ///seq_end 之前的全部重新发送一遍
                Trace("handle nak {}-{}", seq_begin, seq_end);
                _sender_queue->send_again(seq_begin, seq_end);
                continue;
            }
            //// 说明是单个包
            Trace("handle nak {}", seq_begin);
            _sender_queue->send_again(seq_begin, seq_begin);
        }
    }

    /// ack control packet are used to provide the delivery
    void srt_socket_service::handle_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &buff) {
        Trace("handle ack...");
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
            /// 更新rtt rtt_variance.
            Trace("update rtt={}, rtt_variance={}, peer available buffer size={}", _rtt, _rtt_variance, _available_buffer_size);
            _ack_queue_->set_rtt(_rtt, _rtt_variance);
            _sender_queue->update_flow_window(_available_buffer_size);
        }
        /// 滑动序号
        _sender_queue->ack_sequence_to(_last_ack_packet_seq);
    }

    void srt_socket_service::handle_ack_ack(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        auto ack_seq = pkt.get_type_information();
        _ack_queue_->calculate(ack_seq);
        Debug("handle ack ack, rtt={}, rtt_variance={}", _ack_queue_->get_rto(), _ack_queue_->get_rtt_var());
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
        Warn("handle drop packet {}-{}", first_packet_seq, last_packet_seq);
        _receive_queue->drop(first_packet_seq, last_packet_seq);
    }

    void srt_socket_service::handle_shutdown(const srt_packet &pkt, const std::shared_ptr<buffer> &) {
        std::error_code e = make_srt_error(srt_error_code::peer_has_terminated_connection);
        return on_error_in(e);
    }
};// namespace srt
