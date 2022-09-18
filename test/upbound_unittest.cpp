//
// Created by 沈昊 on 2022/8/6.
//
#include <gtest/gtest.h>
#include <set>
#include <vector>

TEST(upbound, upbound) {



    std::vector<int> v = {1, 2, 3, 4, 6, 7, 8};

    std::set<int> sv;
    sv.insert(v.begin(), v.end());
    auto it = sv.upper_bound(4);
    sv.erase(sv.begin(), it);

    auto begin = sv.begin();
    EXPECT_EQ(6, *begin);
    ++begin;
    EXPECT_EQ(7, *begin);
    ++begin;
    EXPECT_EQ(8, *begin);
}