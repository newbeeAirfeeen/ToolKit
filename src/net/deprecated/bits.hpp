/*
* @file_name: bits.hpp
* @date: 2022/08/05
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef TOOLKIT_BITS_HPP
#define TOOLKIT_BITS_HPP
#include <cstdint>

template<size_t L, size_t R, bool parent_correct = true>
struct bit_set_mask {
    static constexpr bool correct = L >= R;
    static constexpr uint32_t value = (1u << L) | bit_set_mask<L - 1, R, correct>::value;
};

template<size_t R>
struct bit_set_mask<R, R, true> {
    static const bool correct = true;
    static const uint32_t value = 1 << R;
};


template<size_t L, size_t R>
struct bit_set_mask<L, R, false> {};

template<size_t L, size_t R = L>
struct bits {
    static constexpr uint32_t mask = bit_set_mask<L, R>::value;
    static constexpr uint32_t offset = R;
    static constexpr size_t size = L - R + 1;

    static bool fit(uint32_t value) {
        return (bit_set_mask<L - R, 0>::value & value) == value;
    }

    static uint32_t wrap(uint32_t base_val) {
        return (base_val << offset) & mask;
    }

    static uint32_t unwrap(uint32_t bitset) {
        return (bitset & mask) >> offset;
    }
};


#endif//TOOLKIT_BITS_HPP
