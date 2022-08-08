//
// Created by 沈昊 on 2022/8/7.
//
#include <gtest/gtest.h>
#include <protocol/srt/srt_extension.h>

bool IsSet(int32_t bitset, int32_t flagset) {
    return (bitset & flagset) == flagset;
}


TEST(XOR, srt) {
    uint32_t xor_val = 0x110000ff;
    uint32_t xor_ = 0x11000000;
    EXPECT_EQ(0x000000FF, xor_val ^ xor_);
    EXPECT_EQ(0, xor_ ^ xor_);
}


TEST(extension, srt) {

    EXPECT_EQ(true, srt::extension_flag(0x0000f000));
    EXPECT_EQ(true, srt::extension_flag(0x00000001));
    EXPECT_NE(true, srt::extension_flag(0x11000000));

    uint32_t expect = 0x00000001 | 0x00000002 | 0x00000004;
    EXPECT_EQ(true, IsSet(expect, 0x00000001));
    EXPECT_EQ(true, IsSet(expect, 0x00000002));
    EXPECT_EQ(true, IsSet(expect, 0x00000004));


    EXPECT_EQ(true, srt::is_HS_REQ_set(expect));
    EXPECT_EQ(true, srt::is_KM_REQ_set(expect));
    EXPECT_EQ(true, srt::is_CONFIG_set(expect));
}

TEST(extension_serialize, srt) {
    using namespace srt;
    uint8_t array[] = {0x00,0x01,/// extension_type
                       0x00,0x03,/// block_size
                       0x00,0x01,0x04,0x04,/// version
                       0x00,0x00,0x00,0xbf,/// flag
                       0x00,0x78,0x00,0x00,/// tsbpd
                       0x00,0x05,0x00,0x02, /// xtension type, block_size
                       0x6e,0x65,0x68,0x73,0x00,0x6f,0x61,0x68};
    auto buff = std::make_shared<buffer>();
    handshake_context ctx;
    auto size = set_extension(ctx, buff, 120, true, true, "shenhao");
    EXPECT_EQ(size, 28);
    const char* pointer = buff->data();
    for(int i = 0;i < 28;i++){
        EXPECT_EQ((const char)array[i], pointer[i]);
    }
}

TEST(extension_serialize_from_buffer, srt){
    uint8_t array[] = {0x00,0x01,/// extension_type
                       0x00,0x03,/// block_size
                       0x00,0x01,0x04,0x04,/// version
                       0x00,0x00,0x00,0xbf,/// flag
                       0x00,0x78,0x00,0x00,/// tsbpd
                       0x00,0x05,0x00,0x02, /// extension type, block_size
                       0x6e,0x65,0x68,0x73,0x00,0x6f,0x61,0x68};
    using namespace srt;
    handshake_context ctx;
    ctx._version = 5;
    ///type
    ctx.extension_field = 5;
    ctx.encryption = 0;
    auto buff = std::make_shared<buffer>();
    buff->append((const char*)array, 28);
    auto ex = get_extension(ctx, buff);
    EXPECT_EQ(28, buff->size());
    EXPECT_EQ(ex->version, 0x010404);
    EXPECT_EQ(ex->stream_id, "shenhao");
    EXPECT_EQ(ex->flags, 0x000000bf);
    EXPECT_EQ(ex->receiver_tlpktd_delay, 120);
    EXPECT_EQ(ex->sender_tlpktd_delay, 0);
    EXPECT_EQ(ex->drop, true);
    EXPECT_EQ(ex->nak, true);

}