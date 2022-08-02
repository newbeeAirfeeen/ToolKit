//
// Created by 沈昊 on 2022/8/2.
//
#include <gtest/gtest.h>
#include <net/buffer.hpp>
#include <protocol/stun/stun.h>
void compare_binary(const std::shared_ptr<buffer> &buf, const char *buff, size_t length) {
    EXPECT_GE(buf->size(), length);
    for (int i = 0; i < length; i++) {
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
        pkt.set_message_integrity(true);
        // tlv = 4 + 8 = 12;
        pkt.set_software("software");

        try {
            pkt_ptr = stun::stun_packet::create_packet(pkt);
        } catch (const std::exception &e) {
            EXPECT_EXIT(1, ::testing::ExitedWithCode(1), e.what());
        }

        /// expect padding
        EXPECT_EQ(pkt_ptr->size() % 4, 0) << "the buffer is  not multiple of 4";
        EXPECT_EQ(pkt_ptr->size(), 88) << "the stun pkt length is not correct";


//        asio::io_context io;
//        asio::ip::udp::socket sock(io);
//        auto stun_req = stun::udp::create_request(sock, pkt, "stun.l.google.com", 19302);
//        stun::udp::send_request(stun_req);
//        io.run();
    };
    EXPECT_NO_THROW(func());
}
