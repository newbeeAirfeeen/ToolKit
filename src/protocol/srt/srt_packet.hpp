﻿/*
* @file_name: srt_packet.hpp
* @date: 2022/04/21
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
#ifndef TOOLKIT_SRT_PACKET_HPP
#define TOOLKIT_SRT_PACKET_HPP
#include <iostream>
#include "net/buffer.hpp"
#include "srt_packet_profile.hpp"
/**
 * The SRT Protocol draft-sharabayko-srt-01
 *
 * Page 6
 * SRT Packet are transmitted as UDP payload [RFC0768], Every UDP packet carrying SRT traffic
 * contains an SRT header immediately after the UDP header
 *
 *                0                   1                   2                   3
 *                0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |             SrcPort           |             DstPort           |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |             Len               |             ChkSum            |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |                                                               |
 *               +                            SRT Packet                         +
 *               |                                                               |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                              Figure 1: SRT packet as UDP payload
 * SRT has two types of packet type distinguished by packet type: data packet and protocol packet
 *
 *               0                   1                   2                   3
 *               0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *               +-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |F|        (Field meaning depends on the packet type)           |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |          (Field meaning depends on the packet type)           |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |                          Timestamp                            |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |                     Destination Socket ID                     |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *               |                                                               |
 *               +                      Packet Contents                          |
 *               |                  (depends on the packet type)                 +
 *               |                                                               |
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
namespace srt{

    struct srt_packet : public basic_buffer<char>{
    public:
        bool is_control_packet() const;
        srt_packet();
        srt_packet(basic_buffer<char>&& buf);
    };

    struct srt_packet_helper{
        static std::shared_ptr<srt_packet> make_srt_control_packet(control_type type, uint32_t timestamp, uint32_t socket_id, uint32_t type_info = 0);
        static std::shared_ptr<srt_packet> make_srt_data_packet();
    };

};

#endif//TOOLKIT_SRT_PACKET_HPP
