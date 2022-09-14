/*
* @file_name: srt_packet.hpp
* @date: 2022/04/21
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
#ifndef TOOLKIT_SRT_PACKET_H
#define TOOLKIT_SRT_PACKET_H
#include "net/buffer.hpp"
#include "srt_control_type.h"
#include <cstdint>
#include <memory>
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
namespace srt {

    class srt_packet {
        friend std::shared_ptr<buffer> create_packet(const srt_packet &) noexcept;
        friend std::shared_ptr<srt_packet> from_buffer(const char *data, size_t length);

    public:
        void set_control(bool is_control);
        void set_timestamp(uint32_t timestamp);
        void set_socket_id(uint32_t sock_id);
        /// data  packet
        ///    /**
        ///    * 0                   1                   2                   3
        ///    * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    * |0|                  Packet Sequence Number                     |
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    * |P P|O|K K|R|            Message Number                         |
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    * |                           Timestamp                           |
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    * |                     Destination Socket ID                     |
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    * |                                                               |
        ///    * +                            Data                               +
        ///    * |                                                               |
        ///    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ///    *               Figure 3: Data packet structure
        ///    */
        void set_packet_sequence_number(uint32_t);
        void set_packet_position_flag(uint8_t);
        void set_in_order(bool on);
        void set_key_encryption_flag(uint8_t encryption);
        void set_retransmitted(bool on);
        void set_message_number(uint32_t time_stamp);
        bool get_control() const;
        uint32_t get_time_stamp() const;
        uint32_t get_socket_id() const;
        uint32_t get_packet_sequence_number() const;
        uint8_t get_packet_position_flag() const;
        bool get_key_based_encryption_flag() const;
        bool is_retransmitted() const;
        bool get_in_order() const;
        uint32_t get_message_number() const;
        void set_data(const char *data, size_t length);
        const std::string &get_data() const;
        /// control packet
        void set_control_type(control_type);
        void set_type_information(uint32_t);
        control_type get_control_type() const;
        uint32_t get_type_information() const;

    private:
        bool is_control = true;
        /// Timestamp: 32 bits. @See section 3
        uint32_t time_stamp = 0;
        /// Destination Socket ID: 32 bits. @See Section 3
        uint32_t destination_socket_id = 0;

    private:
        /// data packet
        /// flags
        /// 3.1 Data packets Page 8
        /// the sequential number of the data packet.
        ///
        uint32_t packet_sequence_number = 0;
        /// PP: 2 bits. Packet Position Flag. This field indicates the position
        /// of the data packet in the message. The value "10b" (binary) means
        /// the first packet of the message. "00b" indicates a packet in the
        /// middle. "01b" designates the last packet. If a single data packet
        /// forms the whole message, the value is "11b".
        uint8_t packet_position_flag = 0b11;
        /// Order Flag. Indicates whether the message should be
        /// delivered by the receiver in order (1) or not (0). Certain
        /// restrictions apply depending on the data transmission mode used
        /// (Section 4.2).
        uint8_t in_order = 0;
        ///  KK: 2 bits. Key-based Encryption Flag. The flag bits indicate
        /// whether or not data is encrypted. The value "00b" (binary) means
        /// data is not encrypted. "01b" indicates that data is encrypted with
        /// an even key, and "10b" is used for odd key encryption. Refer to
        /// Section 6. The value "11b" is only used in control packets.
        uint8_t encryption_flag = 0;
        /// R: 1 bit. Retransmitted Packet Flag. This flag is clear when a
        /// packet is transmitted the first time. The flag is set to "1" when
        /// a packet is retransmitted.
        uint8_t transmitted_packet_flag = 0;
        /// message number: 26 bits, the sequential number of consecutive data
        /// packets that form a message (See PP field).
        uint32_t message_number = 0;
        std::string data;
        /// control packet
    private:
        /**
         * An SRT control packet has the following structure.
         * 0                   1                   2                   3
         * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         * +-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |1|          Control Type      |            Subtype            |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                   Type-specific Information                  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                        Timestamp                             |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                   Destination Socket ID                      |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+- CIF -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                                                              |
         * +                   Control Information Field                  +
         * |                                                              |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *                 Figure 4: Control packet structure
         */

        /// Control Type: 15 bits. Control Packet Type. The use of these bits
        /// is determined by the control packet type definition. See Table 1.
        control_type _control_type_ = control_type::user_defined_type;
        /// Type-specific Information: 32 bits. The use of this field depends o
        /// the particular control packet type. Handshake packets do not use
        /// this field.
        uint32_t type_information = 0;
    };


    std::shared_ptr<buffer> create_packet(const srt_packet &) noexcept;
    std::shared_ptr<srt_packet> from_buffer(const char *data, size_t length);
    void set_packet_timestamp(const std::shared_ptr<buffer> &buff, uint32_t ts);
#if 0
    void update_packet_data_flag(const srt_packet &pkt, const std::shared_ptr<buffer> &ptr) noexcept;
#endif
    void set_retransmit(bool, const std::shared_ptr<buffer> &buff);
};// namespace srt

#endif//TOOLKIT_SRT_PACKET_H
