//
// Created by 沈昊 on 2022/8/6.
//
#include <gtest/gtest.h>
#include "net/deprecated/bits.hpp"
#include <iostream>
using namespace std;



TEST(bits, srt){

    constexpr uint32_t value = bit_set_mask<15, 0>::value;
    constexpr uint32_t value2 =  bit_set_mask<31, 16>::value;
    EXPECT_EQ(value, 0xffff);
    EXPECT_EQ(value2, 0xffff0000);

    uint32_t vv = bits<31, 16>::unwrap(0x11223344);
    uint32_t v = bits<15, 0>::unwrap(0x11223344);
    EXPECT_EQ(0x1122, vv);
    EXPECT_EQ(0x3344, v);

}


