/*
* @file_name: srt_extension.cpp
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
#include "srt_extension.h"
#include "Util/endian.hpp"
#include "srt_error.hpp"
#include <algorithm>

namespace srt {
    constexpr const uint32_t mask = 0xFFFF;
    bool extension_flag(uint32_t flag) {
        return flag & mask;
    }

    bool is_HS_REQ_set(uint32_t flag) {
        return (flag & mask & HS_REQ) == HS_REQ;
    }

    bool is_KM_REQ_set(uint32_t flag) {
        return (flag & mask & KM_REQ) == KM_REQ;
    }

    bool is_CONFIG_set(uint32_t flag) {
        return (flag & mask & CONFIG) == CONFIG;
    }

    bool is_extension_type(uint32_t type) {
        switch (type) {
            case SRT_CMD_HS_REQ:
            case SRT_CMD_HS_RSP:
            case SRT_CMD_KM_REQ:
            case SRT_CMD_KM_RSP:
            case SRT_CMD_SID:
            case SRT_CMD_CONGESTION:
            case SRT_CMD_FILTER:
            case SRT_CMD_GROUP:
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
#if 0
    /// when invoke this, out must be initial
    size_t set_TSBPD_flag(uint32_t peer_ms, char *data, size_t length) {
        if (length < 16 || peer_ms == 0) {
            return 0;
        }

        auto *extension_type = (uint16_t *) data;
        auto *extension_length = (uint16_t *) (data + 2);
        auto *version = (uint32_t *) (data + 4);
        auto *flag = (uint32_t *) (data + 8);
        auto *receiver_delay = (uint16_t *) (data + 12);
        auto *sender_delay = (uint16_t *) (data + 14);

        set_be16(extension_type, static_cast<uint16_t>(SRT_CMD_HS_REQ));
        /// the length of the extension contents field in four-byte blocks
        set_be16(extension_length, static_cast<uint16_t>(3));
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
        if (length < 16) {
            return 0;
        }

        auto *flag = (uint32_t *) (data + 8);
        auto old_flag = load_be32(flag);
        if( on )
            old_flag |= static_cast<uint32_t>(extension_message_flag::TLPKTDROP);
        else
            old_flag ^= static_cast<uint32_t>(extension_message_flag::TLPKTDROP);
        return 4;
    }

    size_t set_PERIODICNAK_flag(bool on, char *data, size_t length) {
        return 0;
    }

    size_t set_REXMIT_flag(char *data, size_t length) {
        return 0;
    }

    void set_config_string(const std::shared_ptr<buffer> &buff, extension_message_flag flag, const std::string &data) {
    }

    void set_config_string(const std::shared_ptr<buffer> &buff, extension_message_flag flag, const char *data, size_t length) {
    }

#endif
    static void set_ext_string(const std::shared_ptr<buffer> &buff, const char *data, uint32_t length) {
        const auto *pointer = (const uint32_t *) data;
        length = length / 4;
        for (uint32_t i = 0; i < length; i++) {
            buff->put_be<uint32_t>(*pointer++);
        }
    }

    static void get_ext_string(const uint32_t *data, uint32_t block_size, std::string &stream_id) {
        size_t i = 0;
        for (i = 0; i < (size_t)block_size; i++) {
            uint32_t raw = load_be32(data++);
            stream_id.append((const char *) &raw, sizeof(uint32_t));
        }
        if (i <= 0) {
            return;
        }
        /// 找到第一次出现0的位置
        i = stream_id.find_first_of(static_cast<char>(0), (i - 1) * 4);
        if (static_cast<size_t>(i) != std::string::npos) {
            stream_id.erase(i);
        }
    }

    size_t set_extension(handshake_context &ctx, const std::shared_ptr<buffer> &buf, uint16_t ts, bool drop, bool nak, uint16_t sender_ts, const std::string &stream_id) {
        auto origin_size = buf->size();
        auto space = 20 + (stream_id.size() + 3) / 4;
        buf->reserve(space);
        /// cache
        char data[16] = {0};
        auto *extension_type = (uint16_t *) data;
        auto *extension_length = (uint16_t *) (data + 2);
        auto *version = (uint32_t *) (data + 4);
        auto *flag = (uint32_t *) (data + 8);
        auto *receiver_delay = (uint16_t *) (data + 12);
        auto *sender_delay = (uint16_t *) (data + 14);
        ctx.extension_field = HS_REQ;
        set_be16(extension_type, static_cast<uint16_t>(SRT_CMD_HS_REQ));
        /// the length of the extension contents field in four-byte blocks
        set_be16(extension_length, static_cast<uint16_t>(3));
        /// SRT 1.4.4
        set_be32(version, 0x010404);
        auto flags = load_be32(flag) | static_cast<uint32_t>(TSBPDRCV) | static_cast<uint32_t>(TSBPDSND);

        /// set drop
        if (drop)
            flags |= static_cast<uint32_t>(TLPKTDROP);
        else
            flags ^= static_cast<uint32_t>(TLPKTDROP);

        /// set nak report
        if (nak)
            flags |= static_cast<uint32_t>(PERIODICNAK);
        else
            flags ^= static_cast<uint32_t>(PERIODICNAK);

        /// set transmit
        flags |= static_cast<uint32_t>(REXMITFLG);

        //// MUST set
        flags |= static_cast<uint32_t>(PACKET_FILTER);
        flags |= static_cast<uint32_t>(CRYPT);

        set_be32(flag, flags);
        set_be16(receiver_delay, static_cast<uint16_t>(ts));
        set_be16(sender_delay, static_cast<uint16_t>(sender_ts));

        buf->append(data, 16);
        /// stream id
        if (!stream_id.empty()) {
            if (stream_id.size() > 728) {
                throw std::system_error(make_srt_error(srt_stream_id_too_long));
            }
            /// 设置config 为 config
            ctx.extension_field |= CONFIG;
            /// 单位为4的字节块
            size_t word_size = (stream_id.size() + 3) / 4;
            /// 填充的字节数
            size_t aligned_byte_size = word_size * 4 - stream_id.size();
            /// 放入extension type
            buf->put_be<uint16_t>(static_cast<uint16_t>(SRT_CMD_SID));
            /// 放入extension length
            buf->put_be<uint16_t>(static_cast<uint16_t>(word_size));
            /// 大小端倒序
            set_ext_string(buf, stream_id.data(), static_cast<uint32_t>(stream_id.size() + aligned_byte_size));
        }
        return buf->size() - origin_size;
    }


    std::shared_ptr<extension_field> get_extension(const handshake_context &ctx, const std::shared_ptr<buffer> &buff) {
        auto extension = std::make_shared<extension_field>();
        /// stream id
        const char set_end[] = {'\0'};
        while (buff->size() > 4) {
            auto extension_type_ = buff->get_be<uint16_t>();
            if (!is_extension_type(extension_type_)) {
                throw std::system_error(make_srt_error(srt_unknown_extension_field));
            }

            auto extension_ = static_cast<extension_type>(extension_type_);
            uint32_t extension_block_size = buff->get_be<uint16_t>() * 4;
            if (buff->size() < extension_block_size) {
                throw std::system_error(make_srt_error(srt_packet_error));
            }
            /// 解析HS_REQ
            if (is_HS_REQ_set(ctx.extension_field) && (extension_ == SRT_CMD_HS_REQ || extension_ == SRT_CMD_HS_RSP)) {
                extension->version = buff->get_be<uint32_t>();
                extension->flags = buff->get_be<uint32_t>();
                /// tlpktd
                if (extension->flags & (TSBPDSND | TSBPDRCV)) {
                    extension->receiver_tlpktd_delay = buff->get_be<uint16_t>();
                    extension->sender_tlpktd_delay = buff->get_be<uint16_t>();
                }
                /// drop
                extension->drop = static_cast<bool>(extension->flags & TLPKTDROP);
                /// nak
                extension->nak = static_cast<bool>(extension->flags & PERIODICNAK);
            }
            /// 不支持加密
            else if (is_KM_REQ_set(ctx.extension_field) && extension_ == SRT_CMD_KM_RSP) {
                throw std::system_error(make_srt_error(srt_KM_REQ_is_not_support));
            } else if (is_CONFIG_set(ctx.extension_field) && (extension_ == SRT_CMD_SID)) {
                get_ext_string((const uint32_t *) buff->data(), extension_block_size / 4, std::ref(extension->stream_id));
                buff->remove(extension_block_size);
            } else {
                /// 其余暂时先忽略
                buff->remove(extension_block_size);
            }
        }
        /// 恢复之
        buff->backward();
        return extension;
    }

}// namespace srt