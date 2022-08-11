//
// Created by 沈昊 on 2022/8/11.
//
#include <gtest/gtest.h>
#include <Util/circular_buffer.hpp>

TEST(circular_buffer, circular_buffer){

    circular_buffer<int> c(5);
    c.emplace_back(1);
    c.emplace_back(2);
    c.emplace_back(3);
    c.emplace_back(4);
    c.emplace_back(5);
    EXPECT_EQ(c.empty(), false);
    EXPECT_EQ(c.size(), 5);
    auto _1 = c.front();
    EXPECT_EQ(_1, 1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(_1, 2);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(3, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(4, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(5, _1);
    c.pop_front();
    EXPECT_EQ(c.empty(), true);


    c.emplace_back(1);
    c.emplace_back(2);
    c.emplace_back(3);
    c.emplace_back(4);
    c.emplace_back(5);
    c.emplace_back(6); /// 6 2 3 4 5


    EXPECT_EQ(c.empty(), false);
    EXPECT_EQ(c.size(), 5);
    _1 = c.front();
    EXPECT_EQ(_1, 6);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(_1, 2);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(3, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(4, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(5, _1);
    c.pop_front();
    EXPECT_EQ(c.empty(), true);


    c.emplace_back(1);
    c.emplace_back(2);
    c.emplace_back(3);
    c.emplace_back(4);
    c.emplace_back(5);
    c.emplace_back(6);
    c.emplace_back(7);/// 6 7 3 4 5
    EXPECT_EQ(c.empty(), false);
    EXPECT_EQ(c.size(), 5);
    _1 = c.front();
    EXPECT_EQ(_1, 7);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(_1, 3);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(4, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(5, _1);
    c.pop_front();
    _1 = c.front();
    EXPECT_EQ(6, _1);
    c.pop_front();
    EXPECT_EQ(c.empty(), true);
}