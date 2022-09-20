//
// Created by 沈昊 on 2022/8/6.
//
#include "Util/string_util.hpp"
#include <gtest/gtest.h>
#include <set>
#include <vector>
TEST(splice, split) {
    string_view view = "   china is good place to live   ;";
    auto vec = string_util::split(view, " ");
    EXPECT_EQ(7, vec.size());
    auto _ = vec.front();
    EXPECT_EQ("china", _);
    EXPECT_EQ(_.size(), 5);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ("is", _);
    EXPECT_EQ(_.size(), 2);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ("good", _);
    EXPECT_EQ(_.size(), 4);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ("place", _);
    EXPECT_EQ(_.size(), 5);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ("to", _);
    EXPECT_EQ(_.size(), 2);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ("live", _);
    EXPECT_EQ(_.size(), 4);
    vec.pop_front();
    _ = vec.front();
    EXPECT_EQ(";", _);
    EXPECT_EQ(_.size(), 1);
    vec.pop_front();
    EXPECT_EQ(vec.empty(), true);
    view = "       ";
    vec = string_util::split(view, " ");
    EXPECT_EQ(0, vec.size());
}

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