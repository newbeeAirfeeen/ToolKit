/*
* @file_name: srt_extension.h
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

#ifndef TOOLKIT_SRT_EXTENSION_H
#define TOOLKIT_SRT_EXTENSION_H
#include "net/buffer.hpp"
#include "srt_handshake.h"
#include <cstdint>
#include <utility>
namespace srt {

    enum extension_flag {
        HS_REQ = 0x00000001,
        KM_REQ = 0X00000002,
        CONFIG = 0X00000004,
    };

    /// At this point the Listener still does not know if the caller
    /// is SRT or UDT, and it responds with the same set of values regardless
    /// of whether the Caller is SRT or UDT.
    /// if the party is SRT, it does interpret the values in version and extension field.
    /// if it receives value 5 in version, it understands that it come from an SRT party.
    /// it also checks the following
    ///     1. whether the extension flags contains the magic value 0x4A17, otherwise the connection is rejected.
    ///     2. whether the encryption flags contains a non-zero value

    bool extension_flag(uint32_t type);
    bool is_HS_REQ_set(uint32_t type);
    bool is_KM_REQ_set(uint32_t type);
    bool is_CONFIG_set(uint32_t type);

    /// +=======+====================+===================+
    /// | Value |     Extension Type | HS Extension Flag |
    /// +=======+====================+===================+
    /// |   1   |     SRT_CMD_HSREQ  |       HSREQ       |
    /// +-------+--------------------+-------------------+
    /// |   2   |     SRT_CMD_HSRSP  |       HSREQ       |
    /// +-------+--------------------+-------------------+
    /// |   3   |     SRT_CMD_KMREQ  |       KMREQ       |
    /// +-------+--------------------+-------------------+
    /// |   4   |     SRT_CMD_KMRSP  |       KMREQ       |
    /// +-------+--------------------+-------------------+
    /// |   5   |     SRT_CMD_SID    |       CONFIG      |
    /// +-------+--------------------+-------------------+
    /// |   6   | SRT_CMD_CONGESTION |       CONFIG      |
    /// +-------+--------------------+-------------------+
    /// |   7   |    SRT_CMD_FILTER  |       CONFIG      |
    /// +-------+--------------------+-------------------+
    /// |   8   |    SRT_CMD_GROUP   |       CONFIG      |
    /// +-------+--------------------+-------------------+
    enum extension_type {
        SRT_NULL_EXTENSION_ = -1,
        SRT_CMD_HS_REQ = 1,
        SRT_CMD_HS_RSP = 2,
        SRT_CMD_KM_REQ = 3,
        SRT_CMD_KM_RSP = 4,
        SRT_CMD_SID = 5,
        SRT_CMD_CONGESTION = 6, /// The Congestion Warning control packet is reserved for future use.
        SRT_CMD_FILTER = 7,
        SRT_CMD_GROUP = 8,
    };

    bool is_extension_type(uint32_t type);
    std::pair<extension_type, uint32_t> find_extension_block(const char *data, size_t length);


    /// +============+===============+
    /// | Bitmask    |      Flag     |
    /// +============+===============+
    /// | 0x00000001 |    TSBPDSND   |
    /// +------------+---------------+
    /// | 0x00000002 |    TSBPDRCV   |
    /// +------------+---------------+
    /// | 0x00000004 |     CRYPT     |
    /// +------------+---------------+
    /// | 0x00000008 |   TLPKTDROP   |
    /// +------------+---------------+
    /// | 0x00000010 |  PERIODICNAK  |
    /// +------------+---------------+
    /// | 0x00000020 |   REXMITFLG   |
    /// +------------+---------------+
    /// | 0x00000040 |    STREAM     |
    /// +------------+---------------+
    /// | 0x00000080 | PACKET_FILTER |
    /// +------------+---------------+

    enum extension_message_flag {
        TSBPDSND = 0x00000001,     /// for sending,
        TSBPDRCV = 0x00000002,     /// for receiving,
        CRYPT = 0x00000004,        /// for crypt.
        TLPKTDROP = 0x00000008,    /// for too late packet drop mechanism,
        PERIODICNAK = 0x00000010,  /// for periodic nak packets
        REXMITFLG = 0x00000020,    /// flag MUST be set.
        STREAM = 0x00000040,       /// flag is identifies transmission mode, if flag is set, buffer mode used, otherwise, message mode.
        PACKET_FILTER = 0x00000080,/// indicates if the peer supports packet filter, MUST be set.
    };
#if 0
    size_t set_TSBPD_flag(uint32_t peer_ms, char *data, size_t length);
    size_t set_TLPKTDROP_flag(bool on, char *data, size_t length);
    size_t set_PERIODICNAK_flag(bool on, char *data, size_t length);
    size_t set_REXMIT_flag(char *data, size_t length);

    void set_config_string(const std::shared_ptr<buffer>& buff, extension_message_flag flag, const std::string& data);
    void set_config_string(const std::shared_ptr<buffer>& buff, extension_message_flag flag, const char* data, size_t length);
#endif

    struct extension_field {
        uint32_t version = 0;
        uint32_t flags = 0;
        uint16_t receiver_tlpktd_delay = 0;
        uint16_t sender_tlpktd_delay = 0;
        bool drop = true;
        bool nak = true;
        std::string stream_id;
    };


    /// HSREQ + STREAM_ID
    size_t set_extension(handshake_context &ctx, const std::shared_ptr<buffer> &buff, uint16_t ts, bool drop = true, bool nak = true, const std::string &stream_id = "");
    std::shared_ptr<extension_field> get_extension(const handshake_context &ctx, const std::shared_ptr<buffer> &buff);

};// namespace srt


#endif//TOOLKIT_SRT_EXTENSION_H
