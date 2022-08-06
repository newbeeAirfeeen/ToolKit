﻿//
// Created by 沈昊 on 2022/8/1.
//


#include "random.hpp"
#include <random>
static constexpr char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::string makeRandStr(int sz, bool printable) {
    std::string ret;
    ret.resize(sz);
    std::mt19937 rng(std::random_device{}());
    for (int i = 0; i < sz; ++i) {
        if (printable) {
            uint32_t x = rng() % (sizeof(CCH) - 1);
            ret[i] = CCH[x];
        } else {
            ret[i] = rng() % 0xFF;
        }
    }
    return ret;
}