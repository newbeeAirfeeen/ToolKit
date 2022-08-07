//
// Created by 沈昊 on 2022/8/7.
//
#include <gtest/gtest.h>
#include <protocol/srt/srt_extension.h>
TEST(extension, srt){

    EXPECT_EQ(true, srt::have_extension_flag(0x0000f000));
    EXPECT_EQ(true, srt::have_extension_flag(0x00000001));
    EXPECT_NE(true, srt::have_extension_flag(0x11000000));

    EXPECT_EQ(true, srt::is_HS_REQ_set(1));
    EXPECT_EQ(true, srt::is_KM_REQ_set(2));
    EXPECT_EQ(true, srt::is_CONFIG_set(4));


//    EXPECT_EQ(true, srt::is_HS_REQ_set(0x111));
//    EXPECT_EQ(true, srt::is_KM_REQ_set(0x111));
//    EXPECT_EQ(true, srt::is_CONFIG_set(0x111));
}