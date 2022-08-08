//
// Created by 沈昊 on 2022/8/6.
//
#include <gtest/gtest.h>
#include <protocol/srt/srt_handshake.h>
#include <protocol/srt/srt_packet.h>
#include <string>
bool compare(const char *data1, size_t len1, const char *data2, size_t len2) {

    EXPECT_EQ(len1, len2);
    EXPECT_NE(data1, nullptr);
    EXPECT_NE(data2, nullptr);

    for (int i = 0; i < len1; i++) {
        EXPECT_EQ(data1[i], data2[i]);
    }


    return true;
}

TEST(srt_packet, srt) {

    using namespace srt;
    srt_packet pkt;
    pkt.set_control(true);
    pkt.set_control_type(handshake);
    pkt.set_timestamp(163);
    pkt.set_socket_id(0);


    uint8_t buf[] = {0x80, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0xa3,
                     0x00, 0x00, 0x00, 0x00};
    auto buff = srt::create_packet(pkt);
    compare(buff->data(), buff->size(), (const char *) buf, 16);

    auto packet = srt::from_buffer((const char *) buf, 16);
    EXPECT_EQ(true, packet->get_control());
    EXPECT_EQ(handshake, packet->get_control_type());
    EXPECT_EQ(163, packet->get_time_stamp());
    EXPECT_EQ(0, packet->get_socket_id());
}

TEST(induction, srt) {
    using namespace srt;
    srt_packet pkt;
    pkt.set_control(true);
    pkt.set_control_type(control_type::handshake);
    pkt.set_timestamp(846);
    pkt.set_socket_id(0);
    pkt.set_type_information(0);
    /// srt_packet

    handshake_context ctx;
    ctx._version = 4;
    ctx.encryption = 0;
    ctx.extension_field = 2;
    ctx._sequence_number = 112334117;
    ctx._max_mss = 1500;
    ctx._window_size = 8192;
    ctx._req_type = srt::handshake_context::urq_induction;
    ctx._socket_id = 1071546736;
    ctx._cookie = 0;
    ctx.address = asio::ip::make_address("127.0.0.1");

    auto _pkt = create_packet(pkt);
    handshake_context::to_buffer(ctx, _pkt);

    uint8_t binary_buf[] = {
            0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x03, 0x4e, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02,
            0x06, 0xb2, 0x15, 0x25, 0x00, 0x00, 0x05, 0xdc,
            0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x3f, 0xde, 0x81, 0x70, 0x00, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x07f, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    const char *pointer = _pkt->data();
    compare((const char *) binary_buf, 64, _pkt->data(), _pkt->size());
}

TEST(indunction_1, srt) {
    uint8_t binary_buf[] = {
            0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xd0, 0x05, 0x9c, 0x95, 0x3f, 0xde, 0x81, 0x70,
            0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x4a, 0x17,
            0x06, 0xb2, 0x15, 0x25, 0x00, 0x00, 0x05, 0xdc,
            0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x3f, 0xde, 0x81, 0x70, 0x74, 0x99, 0x62, 0xc3,
            0x01, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    auto buff = buffer::assign((const char*)binary_buf, 64);
    auto pkt = srt::from_buffer(buff->data(), 16);
    buff->remove(16);
    auto context = srt::handshake_context::from_buffer(buff->data(), buff->size());

    EXPECT_EQ(true, pkt->get_control());
    EXPECT_EQ(pkt->get_control_type(), srt::handshake);
    EXPECT_EQ(0, pkt->get_type_information());
    EXPECT_EQ(3490028693, pkt->get_time_stamp());
    EXPECT_EQ(0x3fde8170, pkt->get_socket_id());

    /// induction
    EXPECT_EQ(context->_version, 5);
    EXPECT_EQ(context->encryption, 0);
    EXPECT_EQ(context->extension_field, 0x4a17);
    EXPECT_EQ(context->_sequence_number, 112334117);
    EXPECT_EQ(context->_max_mss, 1500);
    EXPECT_EQ(context->_window_size, 8192);
    EXPECT_EQ(context->_req_type, srt::handshake_context::urq_induction);
    EXPECT_EQ(context->_socket_id, 1071546736);
    EXPECT_EQ(context->_cookie, 0x749962c3);
    std::string address = context->address.to_string();
    EXPECT_EQ(address, "127.0.0.1");

}