//
// Created by 沈昊 on 2022/8/6.
//
#include <gtest/gtest.h>
#include <protocol/srt/srt_packet.h>
#include <string>
bool compare(const char* data1, size_t len1, const char* data2, size_t len2){

    EXPECT_EQ(len1, len2);
    EXPECT_NE(data1, nullptr);
    EXPECT_NE(data2, nullptr);

    for(int i = 0;i < len1;i++){
        EXPECT_EQ(data1[i], data2[i]);
    }


    return true;
}

TEST(rfind, string){
    const char* str = "shenhao is the best";
    std::string str1 = "shenhao is the best boy";
    str1.erase(str1.rfind(' '));
    compare(str1.data(), str1.size(), str, 19);
    const char* compare_st = "shen";
    char buf[] = {'s', 'h', 'e', 'n', 0, 0, 0, 0};
    str1.assign(buf, 8);
    str1.erase(str1.find_first_of(static_cast<char>(0)));
    compare(str1.data(), str1.size(), compare_st, 4);
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
    compare(buff->data(), buff->size(), (const char*)buf, 16);

    auto packet = srt::from_buffer((const char*)buf, 16);
    EXPECT_EQ(true, packet->get_control());
    EXPECT_EQ(handshake, packet->get_control_type());
    EXPECT_EQ(163, packet->get_time_stamp());
    EXPECT_EQ(0, packet->get_socket_id());


}