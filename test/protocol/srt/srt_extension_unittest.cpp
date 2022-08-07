//
// Created by 沈昊 on 2022/8/7.
//
#include <gtest/gtest.h>
#include <protocol/srt/srt_extension.h>

bool IsSet(int32_t bitset, int32_t flagset)
{
    return (bitset & flagset) == flagset;
}


TEST(XOR, srt){
    uint32_t xor_val = 0x110000ff;
    uint32_t xor_ = 0x11000000;
    EXPECT_EQ(0x000000FF, xor_val ^ xor_);
    EXPECT_EQ(0, xor_ ^ xor_);
}


TEST(extension, srt){

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