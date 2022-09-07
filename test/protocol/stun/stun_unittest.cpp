//
// Created by 沈昊 on 2022/8/2.
//
#include <gtest/gtest.h>
#include "net/asio.hpp"
#include <net/buffer.hpp>
#include <protocol/stun/stun.h>
void compare_binary(const std::shared_ptr<buffer> &buf, const char *buff, size_t length) {
    EXPECT_GE(buf->size(), length);
    for (size_t i = 0; i < length; i++) {
        EXPECT_EQ(buf->operator[](i), buff[i]);
    }
}


TEST(basic_binding_request, stun) {
    unsigned char pkt_bin[] = {0x00, 0x01,             /// type
                               0x00, 0x00,             /// length
                               0x21, 0x12, 0xA4, 0x42};///  magic cookie

    stun::stun_packet pkt;
    auto pkt_ptr = stun::stun_packet::create_packet(pkt);
    compare_binary(pkt_ptr, (const char *) pkt_bin, 8);
    EXPECT_EQ(pkt_ptr->size(), 20);
}


TEST(attribute, stun) {
    std::shared_ptr<buffer> pkt_ptr;
    auto func = [&]() {
        stun::stun_packet pkt;

        // tlv = 4 + 4 = 8
        pkt.set_finger_print(true);
        // tlv = 4 + 8 = 12;
        pkt.set_username("shenhao");
        pkt.set_password("123456");
        // tlv = 4 + 8 = 12;
        pkt.set_realm("realm");
        // tlv = 4 + 20 = 24;
#ifdef WOLFSSL_ENABLE
        pkt.set_message_integrity(true);
#endif
        // tlv = 4 + 8 = 12;
        pkt.set_software("software");

        pkt_ptr = stun::stun_packet::create_packet(pkt);

        /// expect padding
        EXPECT_EQ(pkt_ptr->size() % 4, 0) << "the buffer is  not multiple of 4";
        EXPECT_EQ(pkt_ptr->size(), 88) << "the stun pkt length is not correct";
    };
    func();
}


TEST(xor_mapped_address, stun) {
    uint8_t buf[] = {0x01, 0x01, 0x00, 0x0c, 0x21, 0x12,
                     0xa4, 0x42, 0x47, 0x4d, 0xcc, 0xe6,
                     0x79, 0x4a, 0x14, 0x4c, 0x64, 0xd9, 0xc1,
                     0x8d, 0x00, 0x20, 0x00, 0x08, 0x00, 0x01, 0x1c, 0x59,
                     0x1d, 0x1e, 0x55, 0xe8};
    const std::string transaction_id = "\x47\x4d\xcc\xe6\x79\x4a\x14\x4c\x64\xd9\xc1\x8d";

    using namespace stun;
    /// 60.12.241.170:15691
    stun_packet pkt;
    pkt.set_method(stun_method::binding_response);
    pkt.set_xor_mapped_address("60.12.241.170", 15691);
    auto pkt_ptr = stun_packet::create_packet(pkt);
    char *p = (char *) (pkt_ptr->data() + 8);
    std::copy(transaction_id.begin(), transaction_id.end(), p);
    compare_binary(pkt_ptr, (const char *) buf, 32);
}
