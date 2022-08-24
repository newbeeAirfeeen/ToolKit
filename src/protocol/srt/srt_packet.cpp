/*
* @file_name: srt_packet.cpp
* @date: 2022/04/27
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
#include "srt_packet.h"
#include "Util/endian.hpp"
#include "srt_control_type.h"
#include "srt_error.hpp"
namespace srt {
    void srt_packet::set_control(bool is_control) {
        this->is_control = is_control;
    }

    void srt_packet::set_timestamp(uint32_t timestamp) {
        this->time_stamp = timestamp;
    }

    void srt_packet::set_socket_id(uint32_t sock_id) {
        this->destination_socket_id = sock_id;
    }

    void srt_packet::set_packet_sequence_number(uint32_t number) {
        this->packet_sequence_number = number;
    }

    void srt_packet::set_packet_position_flag(uint8_t flag) {
        this->packet_position_flag = flag;
    }

    void srt_packet::set_in_order(bool on) {
        this->in_order = static_cast<bool>(on);
    }

    void srt_packet::set_key_encryption_flag(uint8_t encryption) {
        this->encryption_flag = encryption;
    }

    void srt_packet::set_retransmitted(bool on) {
        this->transmitted_packet_flag = on;
    }

    void srt_packet::set_message_number(uint32_t message_number) {
        this->message_number = message_number;
    }

    bool srt_packet::get_control() const {
        return this->is_control;
    }

    uint32_t srt_packet::get_time_stamp() const {
        return this->time_stamp;
    }

    uint32_t srt_packet::get_socket_id() const {
        return this->destination_socket_id;
    }

    uint32_t srt_packet::get_packet_sequence_number() const {
        return this->packet_sequence_number;
    }

    uint8_t srt_packet::get_packet_position_flag() const {
        return this->packet_position_flag;
    }

    bool srt_packet::get_key_based_encryption_flag() const {
        return this->encryption_flag;
    }

    bool srt_packet::is_retransmitted() const {
        return this->transmitted_packet_flag;
    }

    bool srt_packet::get_in_order() const {
        return this->in_order;
    }

    uint32_t srt_packet::get_message_number() const {
        return this->message_number;
    }

    void srt_packet::set_data(const char *_data, size_t length) {
        this->data.assign(_data, length);
    }

    const std::string &srt_packet::get_data() const {
        return this->data;
    }

    void srt_packet::set_control_type(control_type type) {
        this->_control_type_ = type;
    }

    void srt_packet::set_type_information(uint32_t type_information) {
        this->type_information = type_information;
    }

    control_type srt_packet::get_control_type() const {
        return this->_control_type_;
    }

    uint32_t srt_packet::get_type_information() const {
        return this->type_information;
    }


    std::shared_ptr<buffer> create_packet(const srt_packet &pkt) noexcept {
        auto buff = std::make_shared<buffer>();
        buff->reserve(1500);
        if (pkt.is_control) {
            /// control type
            buff->put_be<uint16_t>(static_cast<uint16_t>(pkt.get_control_type()) | 0x8000);
            /// sub type
            buff->put_be<uint16_t>(static_cast<uint16_t>(0));
            buff->put_be<uint32_t>(pkt.get_type_information());
        } else {
            /// sequence number
            buff->put_be<uint32_t>(pkt.get_packet_sequence_number() & 0x7FFFFFFF);
            /// flag + message_number
            uint8_t flag = (pkt.get_packet_position_flag() << 6) | (pkt.get_key_based_encryption_flag() << 3) | (pkt.is_retransmitted() << 2);
            uint32_t message_number = (flag << 24) | pkt.get_message_number();
            buff->put_be<uint32_t>(message_number);
        }
        /// timestamp
        buff->put_be<uint32_t>(pkt.get_time_stamp());
        /// destination socket id
        buff->put_be<uint32_t>(pkt.get_socket_id());
        /// data
        if (!pkt.get_data().empty())
            buff->append(pkt.get_data());
        return buff;
    }

    std::shared_ptr<srt_packet> from_buffer(const char *data, size_t length) {
        if (length < 16) {
            throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
        }
        auto pkt = std::make_shared<srt_packet>();
        bool control = data[0] & 0x80;
        pkt->set_control(control);
        if (control) {
            uint16_t _control_type = load_be16(data) & 0x7FFF;
            if (!is_control_type(_control_type)) {
                throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
            }
            pkt->set_control_type(static_cast<control_type>(_control_type));
            pkt->set_type_information(load_be32(data + 4));
        } else {
            pkt->set_packet_sequence_number(load_be32(data));
            uint8_t position = data[5] & 0xc0;
            uint8_t in_order = data[5] & 0x20;
            uint8_t key_entry = data[5] & 0x18;
            uint8_t retransmit = data[5] & 0x04;
            pkt->set_packet_position_flag(position);
            pkt->set_in_order(static_cast<bool>(in_order));
            pkt->set_key_encryption_flag(key_entry);
            pkt->set_retransmitted(static_cast<bool>(retransmit));
            pkt->set_message_number(load_be32(data + 4) & 0x3FFFFFFF);
        }

        pkt->set_timestamp(load_be32(data + 8));
        pkt->set_socket_id(load_be32(data + 12));
        /// pkt->set_data(data + 16, length - 16);
        return pkt;
    }

    void set_packet_timestamp(const std::shared_ptr<buffer> &buff, uint32_t ts) {
        if (buff->size() < 16) {
            throw std::system_error(make_srt_error(srt_error_code::srt_packet_error));
        }
        char *data = (char *) buff->data() + 8;
        set_be32(data, ts);
    }
#if 0
    void update_packet_data_flag(const srt_packet &pkt, const std::shared_ptr<buffer> &ptr) noexcept {
        if (pkt.get_control()) {
            return;
        }

        if (!ptr) {
            return;
        }

        if (ptr->size() < 8) {
            return;
        }

        uint32_t flag = (pkt.get_packet_position_flag() << 6) | (pkt.get_key_based_encryption_flag() << 3) | (pkt.is_retransmitted() << 2);
        uint32_t message_number = (flag << 26) | (pkt.get_message_number() | 0x3FFFFFFF);

        char *pointer = (char *) ptr->data() + 4;
        set_be32(pointer, message_number);
    }
#endif
    void set_retransmit(bool on, const std::shared_ptr<buffer> &buff) {
        if (!buff) {
            return;
        }
        uint8_t *p = ((uint8_t *) buff->data()) + 4;
        if (on)
            *p = (*p) | 0x04;
        else
            *(p) = (*p) ^ 0x04;
    }
};// namespace srt