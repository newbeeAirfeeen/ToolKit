/*
* @file_name: srt_handshake.hpp
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
#ifndef TOOLKIT_SRT_HANDSHAKE_HPP
#define TOOLKIT_SRT_HANDSHAKE_HPP
#include <net/buffer.hpp>
#include "srt_packet_common.hpp"
#include <tuple>
namespace srt{

    class handshake_packet_base{
    public:
        /**
         * Handshake Type Page 13.
         */
        enum packet_type{
            urq_done        = 0xFFFFFFFD,
            urq_induction   = 0x00000001, // First part for client-server connection
            urq_wave_a_hand = 0x00000000, // First part for rendezvous connection
            urq_conclusion  = 0xFFFFFFFF, // Second part of handshake negotiation
            urq_agreement   = 0xFFFFFFFE,
        };
    };

    struct handshake_packet: public handshake_packet_base{
        using pointer = std::shared_ptr<handshake_packet>;
        static constexpr const uint32_t packet_size = 48;
        bool load(const control_packet_context& ctx, const srt_packet& pkt);
        uint32_t _version = 0; //UDT version
        uint16_t encryption = 0;
        uint32_t _sequence_number = 0; //random initial sequence number
        uint32_t _max_mss = 0; //maximum segment size MTU
        uint32_t _window_size = 0; //flow control window size
        packet_type _req_type = urq_done;
        uint32_t _socket_id = 0;
        uint32_t _cookie = 0;
        uint8_t  _peer_ip[16] = {0};
    };
};
#endif//TOOLKIT_SRT_HANDSHAKE_HPP
