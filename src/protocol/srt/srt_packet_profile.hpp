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
#include "net/buffer.hpp"
#include "srt_packet_common.hpp"
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
    struct control_packet_context{
    public:
        explicit control_packet_context(const srt_packet& pkt);
        control_type get_control_type() const;
        unsigned short get_sub_type() const;
        uint32_t get_type_information() const;
        /**
         * 3. Packet Structure Page 7
         *    32 bits, the timestamp of the packet, in microseconds. the value is
         *    relative to the time the SRT connection was established.
         */
         uint32_t get_timestamp() const;
        /**
         * 3. Packet Structure Page 7
         *    32 bits, A fixed-width field providing the SRT socket ID to which a packet
         *    should be dispatched.the field may have the special "0" when the packet is a
         *    connection request.
         */
         uint32_t get_socket_id() const;
         const char* data() const;
         size_t size() const;
    private:
        /**
         * Control Type: 15 bits. Control Packet Type. The use of these bits
         * is determined by the control packet type definition. See Table 1.
         */
        control_type type = control_type::user_defined_type;
        /**
         * Subtype: 16 bits. This field specifies an additional subtype for
         * specific packets. See Table 1.
         */
        unsigned short sub_type = 0;
        /**
         * Type-specific Information: 32 bits. The use of this field depends on
         * the particular control packet type. Handshake packets do not use
         * this field.
         */
        uint32_t type_information = 0;
        /**
         * @See Section 3
         */
        uint32_t time_stamp = 0;
        /**
         * @See Section 3
         */
        uint32_t socket_id = 0;
        const char* raw_data = nullptr;
        size_t raw_size = 0;
    };

    /**
     * 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |0|                  Packet Sequence Number                     |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |P P|O|K K|R|            Message Number                         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                           Timestamp                           |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                     Destination Socket ID                     |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                                                               |
     * +                            Data                               +
     * |                                                               |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *               Figure 3: Data packet structure
     */
    struct data_packet_context{
    public:
        explicit data_packet_context(const srt_packet& pkt);
        uint32_t get_sequence() const;
        uint32_t get_message_sequence() const;
        uint32_t get_timestamp() const;
        uint32_t get_socket_id() const;
        bool is_first_packet() const;
        bool is_single_packet() const;
        bool is_middle_packet() const;
        bool is_last_packet() const;
        bool should_order() const;
        int key_encrypt() const;
        bool is_retransmitted() const;
        const char* data() const;
        size_t size() const;
    private:
        /**
         * 3.1 Data packets Page 8
         * the sequential number of the data packet.
         */
        uint32_t packet_sequence_number = 0;
        /**
         * message number: 26 bits, the sequential number of consecutive data
         * packets that form a message (See PP field).
         */
        uint32_t message_number = 0;
        /**
         * Timestamp: 32 bits. @See section 3
         */
        uint32_t time_stamp = 0;
        /**
         * Destination Socket ID: 32 bits. @See Section 3
         */
        uint32_t socket_id = 0;
        const char* raw_data = nullptr;
        size_t length = 0;
        /**
         * PP: 2 bits. Packet Position Flag. This field indicates the position
         * of the data packet in the message. The value "10b" (binary) means
         * the first packet of the message. "00b" indicates a packet in the
         * middle. "01b" designates the last packet. If a single data packet
         * forms the whole message, the value is "11b".
         */
        char packet_position = 0;
        /**
         * Order Flag. Indicates whether the message should be
         * delivered by the receiver in order (1) or not (0). Certain
         * restrictions apply depending on the data transmission mode used
         * (Section 4.2).
         */
        bool should_order_ = true;
        /**
         * KK: 2 bits. Key-based Encryption Flag. The flag bits indicate
         * whether or not data is encrypted. The value "00b" (binary) means
         * data is not encrypted. "01b" indicates that data is encrypted with
         * an even key, and "10b" is used for odd key encryption. Refer to
         * Section 6. The value "11b" is only used in control packets.
         */
        char key_encrypt_ = 0;
        /**
         * R: 1 bit. Retransmitted Packet Flag. This flag is clear when a
         * packet is transmitted the first time. The flag is set to "1" when
         * a packet is retransmitted.
         */
        bool is_retransmitted_ = false;
    };

    struct packet_context{
        explicit packet_context(const srt_packet& pkt);

        const data_packet_context& load_as_data() const;
        const control_packet_context& load_as_control() const;
    private:
        data_packet_context* data_ptr = nullptr;
        control_packet_context* control_ptr = nullptr;
    };

    struct ack{
        static bool value(uint32_t val);
    };

    struct ack_ack{
        static bool value(uint32_t val);
    };
};
#endif//TOOLKIT_SRT_PACKET_PROFILE_HPP
