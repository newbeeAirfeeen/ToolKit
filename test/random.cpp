//
// Created by 沈昊 on 2022/8/8.
//
#include <gtest/gtest.h>
#include <random>
#include <set>
TEST(uin32, random){
    std::multiset<uint32_t> s;
    for(int i = 0; i <= 1000;i++){
        std::mt19937 mt(std::random_device{}());
        s.insert(mt());
    }

    for(auto item : s){
        EXPECT_EQ(1, s.count(item));
    }
}