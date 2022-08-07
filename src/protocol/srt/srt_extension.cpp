/*
* @file_name: srt_extension.cpp
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
#include "srt_extension.h"
#include "Util/endian.hpp"
namespace srt {

    bool extension_flag(uint32_t type_information) {
        return type_information & static_cast<uint32_t>(0xFFFF);
    }

    bool is_HS_REQ_set(uint32_t type_information) {
        return (type_information & static_cast<uint32_t>(0xFFFF) & extension_flag::HS_REQ) == extension_flag::HS_REQ;
    }

    bool is_KM_REQ_set(uint32_t type_information) {
        return (type_information & static_cast<uint32_t>(0xFFFF) & extension_flag::KM_REQ) == extension_flag::KM_REQ;
    }

    bool is_CONFIG_set(uint32_t type_information) {
        return (type_information & static_cast<uint32_t>(0xFFFF) & extension_flag::CONFIG) == extension_flag::CONFIG;
    }

    bool is_extension_type(uint32_t type) {
        switch (type) {
            case extension_type::SRT_CMD_HS_REQ:
            case extension_type::SRT_CMD_HS_RSP:
            case extension_type::SRT_CMD_KM_REQ:
            case extension_type::SRT_CMD_KM_RSP:
            case extension_type::SRT_CMD_SID:
            case extension_type::SRT_CMD_CONGESTION:
            case extension_type::SRT_CMD_FILTER:
            case extension_type::SRT_CMD_GROUP:
                return true;
            default:
                return false;
        }
    }

    std::pair<extension_type, uint32_t> find_extension_block(const char *data, size_t length) {
        if (length < 4) {
            return {SRT_NULL_EXTENSION_, 0};
        }
        uint16_t type = load_be16(data);
        uint32_t block_size = load_be16(data + 2) * 4;
        if (!is_extension_type(type)) {
            return {SRT_NULL_EXTENSION_, 0};
        }

        if (length - 4 < block_size) {
            return {SRT_NULL_EXTENSION_, 0};
        }
        return {static_cast<extension_type>(type), block_size};
    }
    /// when invoke this, out must be initial
    size_t set_TSBPD_flag(uint32_t peer_ms, char *out, size_t length) {
        if (length < 16) {
            return 0;
        }

        auto *exten_type = (uint16_t *) out;
        auto *ex_length = (uint16_t *) (out + 2);
        auto *version = (uint32_t *) out + 4;
        auto *flag = (uint32_t *) (out + 8);
        auto *receiver_delay = (uint16_t *) (out + 12);
        auto *sender_delay = (uint16_t *) (out + 14);

        set_be16(exten_type, static_cast<uint16_t>(SRT_CMD_HS_REQ));
        /// the length of the extension contents field in four-byte blocks
        set_be16(ex_length, static_cast<uint16_t>(3));
        /// SRT 1.4.4
        set_be32(version, 0x010404);
        auto old_flag = load_be32(flag);
        /// set TSBPDSND and TSBPDRCV
        set_be32(flag, old_flag | TSBPDSND | TSBPDRCV);
        set_be16(receiver_delay, static_cast<uint16_t>(peer_ms));
        set_be16(sender_delay, static_cast<uint16_t>(0));
        return 16;
    }

    size_t set_TLPKTDROP_flag(bool on, char *data, size_t length) {
        return 0;
    }

    size_t set_PERIODICNAK_flag(bool on, char *data, size_t length) {
        return 0;
    }

    size_t set_REXMIT_flag(char *data, size_t length) {
        return 0;
    }

    size_t flag_helper::set_flag(char *data, size_t length, uint32_t TSBPD, bool TLPKDROP, bool PERIODICNAK) {
    }

    void flag_helper::get_flag(const char *data, size_t length, uint32_t &TSBPD, bool &TLPKDROP, bool &PERIODICNAK) {
    }
}// namespace srt