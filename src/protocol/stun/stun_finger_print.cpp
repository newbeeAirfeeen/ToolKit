//
// Created by 沈昊 on 2022/8/1.
//
#include "stun_finger_print.h"
#include "Util/crc32.hpp"
#include "Util/endian.hpp"
#include "net/buffer.hpp"
#include "stun_packet.h"
#include <cstdint>
namespace stun {

    constexpr int finger_print_size = 4;
    constexpr int attribute_header_size = 4;
    constexpr uint32_t finger_print_xor_value = 0x5354554e;

    void put_finger_print(const stun_packet &packet, const std::shared_ptr<buffer> &buf) {
        /// this must be called in last add attribute
        /// TLV
        if (buf->size() < 4) {
            throw std::bad_function_call();
        }
        /// get the origin length
        auto origin_length = load_be16(buf->data() + 2);
        /// write the new length
        auto new_length = origin_length + finger_print_size + attribute_header_size;
        set_be16((void *) (buf->data() + 2), new_length);
        /// calculate the finger_print
        /// CRC-32 of the STUN message
        /// up to (but excluding) the FINGERPRINT attribute itself, XOR’ed with
        /// the 32-bit value 0x5354554e (the XOR helps in cases where an
        /// application packet is also using CRC-32 in it)
        uint32_t finger_print_ = crc32((const uint8_t *) buf->data(), buf->size()) ^ finger_print_xor_value;
        /// set big endian
        set_be32(&finger_print_, finger_print_);
        /// recover the origin length
        set_be16((void *) (buf->data() + 2), origin_length);

        /// add attribute
        attribute_type attr;
        attr.attribute = finger_print;
        attr.length = finger_print_size;
        attr.value.assign((const char *) &finger_print_, 4);
        /// add attribute
        stun_add_attribute(buf, attr);
    }
};// namespace stun