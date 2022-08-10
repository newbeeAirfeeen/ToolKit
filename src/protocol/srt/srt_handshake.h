/*
* @file_name: srt_handshake.h
* @date: 2022/04/29
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
#ifndef TOOLKIT_SRT_HANDSHAKE_H
#define TOOLKIT_SRT_HANDSHAKE_H
#include "net/asio.hpp"
#include <memory>
#include <net/buffer.hpp>
namespace srt {

    class handshake_context {
    public:
        enum packet_type {
            urq_done = 0xFFFFFFFD,
            urq_induction = 0x00000001,  // First part for client-server connection
            urq_wave_a_hand = 0x00000000,// First part for rendezvous connection
            urq_conclusion = 0xFFFFFFFF, // Second part of handshake negotiation
            urq_agreement = 0xFFFFFFFE,
            rej_unknown = 1000,
            rej_system = 1001,
            rej_peer = 1002,
            rej_resource = 1003,
            rej_rogue = 1004,
            rej_backlog = 1005,
            rej_rej_ipe = 1006,
            rej_close = 1007,
            rej_version = 1008,
            rej_rdv_cookie = 1009,
            rej_bad_secret = 1010,
            rej_unsecure = 1011,
            rej_message_api = 1012,
            rej_congestion = 1013,
            rej_filter = 1014,
            rej_group = 1015,
        };

    public:
        static constexpr const uint32_t packet_size = 48;

    public:
        uint32_t _version = 0;//UDT version
        uint16_t encryption = 0;
        uint16_t extension_field = 0;
        uint32_t _sequence_number = 0;//random initial sequence number
        uint32_t _max_mss = 0;        //maximum segment size MTU
        uint32_t _window_size = 0;    //flow control window size
        packet_type _req_type = urq_done;
        uint32_t _socket_id = 0;
        uint32_t _cookie = 0;
        asio::ip::address address;

        static void update_extension_field(const handshake_context &ctx, const std::shared_ptr<buffer> &b);
        static std::shared_ptr<handshake_context> from_buffer(const char *data, size_t length) noexcept;
        static std::string to_buffer_1(const handshake_context &_handshake) noexcept;
        static size_t to_buffer(const handshake_context &_handshake, char *out, size_t length) noexcept;
        static void to_buffer(const handshake_context &ctx, const std::shared_ptr<buffer> &buff) noexcept;
    };

    bool is_handshake_packet_type(uint32_t);
    const char* get_reject_reason(int e);
};    // namespace srt
#endif//TOOLKIT_SRT_HANDSHAKE_H
