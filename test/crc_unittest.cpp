//
// Created by 沈昊 on 2022/8/2.
//
#include <gtest/gtest.h>
#include <Util/crc32.hpp>

TEST(crc32, crc32){

    uint32_t value = crc32((const uint8_t*)"s", 1);
    EXPECT_EQ(value, 0x1B0ECF0B);


    uint32_t value2 = crc32((const uint8_t*)"shenhao", 7);
    EXPECT_EQ(value2, 0xC94F5C45);


    uint32_t value3 = crc32((const uint8_t*)"\x00\x01\x02\x03\x04\x05\x06", 7);
    EXPECT_EQ(value3, 0xAD5809F9);

}