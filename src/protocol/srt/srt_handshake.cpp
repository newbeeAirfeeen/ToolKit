/*
* @file_name: srt_handshake.cpp
* @date: 2022/04/29
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
#include "srt_handshake.h"
#include "Util/endian.hpp"
#include "srt_control_type.h"
#include "srt_error.hpp"
#include "srt_packet.h"
namespace srt {

    bool is_handshake_packet_type(uint32_t p) {
        switch (p) {
            case handshake_context::urq_induction:
            case handshake_context::urq_wave_a_hand:
            case handshake_context::urq_conclusion:
            case handshake_context::urq_agreement:
                return true;
            default: {
                if (p >= 1000 && p <= 1015)
                    return true;
                return false;
            }
        }
    }

    void handshake_context::update_extension_field(const handshake_context &ctx, const std::shared_ptr<buffer> &b) {
        if (b->size() < 28) {
            throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
        }
        char *p = (char *) b->data() + 22;
        set_be16(p, ctx.extension_field);
    }


    std::shared_ptr<handshake_context> handshake_context::from_buffer(const char *data, size_t length) {
        if (length < 48) {
            throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
        }
        const auto *pointer = (const uint32_t *) data;
        auto handshake = std::make_shared<handshake_context>();
        handshake->_version = load_be32(pointer++);
        const auto *p = (const uint16_t *) pointer;
        handshake->encryption = load_be16(p++);
        handshake->extension_field = load_be16(p);
        pointer++;
        handshake->_sequence_number = load_be32(pointer++);
        handshake->_max_mss = load_be32(pointer++);
        handshake->_window_size = load_be32(pointer++);
        handshake->_req_type = static_cast<handshake_context::packet_type>(load_be32(pointer++));
        if (!is_handshake_packet_type(handshake->_req_type)) {
            throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
        }

        handshake->_socket_id = load_be32(pointer++);
        handshake->_cookie = load_be32(pointer++);
        /// ipv4
        if (pointer[1] == 0 && pointer[2] == 0 && pointer[3] == 0) {
            handshake->address = asio::ip::make_address_v4(*pointer);
        }
        /// ipv6
        else {

            std::string address((const char *) pointer, 16);
            std::array<unsigned char, 16> arr = {0};
            std::copy(address.begin(), address.end(), arr.begin());
            handshake->address = asio::ip::make_address_v6(arr);
        }
        return handshake;
    }

    std::string handshake_context::to_buffer_1(const handshake_context &_handshake) noexcept {
        std::string data;
        data.resize(48);
        to_buffer(_handshake, (char *) data.data(), data.size());
        return data;
    }

    size_t handshake_context::to_buffer(const handshake_context &_handshake, char *out, size_t length) noexcept {
        if (length < 48) {
            return 0;
        }

        auto *pointer = (uint32_t *) out;
        set_be32(pointer++, _handshake._version);
        auto *p = (uint16_t *) pointer;
        set_be16(p++, _handshake.encryption);
        set_be16(p, _handshake.extension_field);
        ++pointer;
        set_be32(pointer++, _handshake._sequence_number);
        set_be32(pointer++, _handshake._max_mss);
        set_be32(pointer++, _handshake._window_size);
        set_be32(pointer++, static_cast<uint32_t>(_handshake._req_type));
        set_be32(pointer++, _handshake._socket_id);
        set_be32(pointer++, _handshake._cookie);
        if (_handshake.address.is_v4()) {
            /// 主机字节序
            auto val = _handshake.address.to_v4().to_uint();
            set_be32(&val, val);
            set_be32(pointer++, val);
            set_be32(pointer++, 0);
            set_be32(pointer++, 0);
            set_be32(pointer++, 0);
        } else {
            /// 主机字节序
            auto *ap = (unsigned char *) pointer;
            auto ad = _handshake.address.to_v6().to_bytes();
            std::copy(ad.rbegin(), ad.rend(), ap);
            pointer += 4;
        }
        return 48;
    }

    void handshake_context::to_buffer(const handshake_context &ctx, const std::shared_ptr<buffer> &buff) noexcept {
        std::string data;
        data.resize(48);
        to_buffer(ctx, (char *) data.data(), data.size());
        buff->append(data);
    }

    const char *get_reject_reason(int e) {
        switch (e) {
            case handshake_context::packet_type::rej_unknown:
                return "Unknown reason";
            case handshake_context::packet_type::rej_system:
                return "System function error";
            case handshake_context::packet_type::rej_peer:
                return "Rejected by peer";
            case handshake_context::packet_type::rej_resource:
                return "Resource allocation problem";
            case handshake_context::packet_type::rej_rogue:
                return "incorrect data in handshake";
            case handshake_context::packet_type::rej_backlog:
                return "listener's backlog exceeded";
            case handshake_context::packet_type::rej_rej_ipe:
                return "internal program error";
            case handshake_context::packet_type::rej_close:
                return "socket is closing";
            case handshake_context::packet_type::rej_version:
                return "peer is older version than agent's min";
            case handshake_context::packet_type::rej_rdv_cookie:
                return "rendezvous cookie collision";
            case handshake_context::packet_type::rej_bad_secret:
                return " wrong password";
            case handshake_context::packet_type::rej_unsecure:
                return "password required or unexpected";
            case handshake_context::packet_type::rej_message_api:
                return "Stream flag collision";
            case handshake_context::packet_type::rej_congestion:
                return "incompatible congestion-controller type";
            case handshake_context::packet_type::rej_filter:
                return "incompatible packet filter";
            case handshake_context::packet_type::rej_group:
                return "incompatible group";
            default:
                return "unknown";
        }
    }
};// namespace srt