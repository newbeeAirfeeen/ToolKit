/*
* @file_name: srt_packet_profile.hpp
* @date: 2022/04/27
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
#ifndef TOOLKIT_SRT_PACKET_PROFILE_HPP
#define TOOLKIT_SRT_PACKET_PROFILE_HPP
#include <iostream>
namespace srt{
    /**
     * +====================+==============+=========+======================+
     * | Packet Type        |  Control Type  |   Subtype   |    Section     |
     * +====================+==============+=========+======================+
     * |    HANDSHAKE       |    0x0000      |     0x0     | Section 3.2.1  |
     * +--------------------+--------------+---------+----------------------+
     * |    KEEPALIVE       |    0x0001      |     0x0     | Section 3.2.3  |
     * +--------------------+--------------+---------+----------------------+
     * |         ACK        |    0x0002      |     0x0     | Section 3.2.4  |
     * +--------------------+--------------+---------+----------------------+
     * | NAK (Loss Report)  |    0x0003      |     0x0     | Section 3.2.5  |
     * +--------------------+--------------+---------+----------------------+
     * | Congestion Warning |    0x0004      |     0x0     | Section 3.2.6  |
     * +--------------------+--------------+---------+----------------------+
     * |     SHUTDOWN       |    0x0005      |     0x0     | Section 3.2.7  |
     * +--------------------+--------------+---------+----------------------+
     * |      ACKACK        |    0x0006      |     0x0     | Section 3.2.8  |
     * +--------------------+--------------+---------+----------------------+
     * |     DROPREQ        |    0x0007      |     0x0     | Section 3.2.9  |
     * +--------------------+--------------+---------+----------------------+
     * |     PEERERROR      |    0x0008      |     0x0     | Section 3.2.10 |
     * +--------------------+--------------+---------+----------------------+
     * | User-Defined Type  |    0x7FFF      |      -      |     N/A        |
     * +--------------------+--------------+---------+----------------------+
     */
    enum control_type{
        handshake          = 0x0000,
        keepalive          = 0x0001,
        ack                = 0x0002,
        nak                = 0x0003,
        congestion_warning = 0x0004,
        shutdown           = 0x0005,
        ack_ack            = 0x0006,
        dro_preq           = 0x0007,
        peer_error         = 0x0008,
        user_defined_type  = 0x7FFF,
    };

    struct control_packet{
        static bool value(uint32_t val);
    };

    struct ack{
        static bool value(uint32_t val);
    };

    struct ack_ack{
        static bool value(uint32_t val);
    };
};
#endif//TOOLKIT_SRT_PACKET_PROFILE_HPP
