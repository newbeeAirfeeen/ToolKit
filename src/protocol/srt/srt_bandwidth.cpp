/*
* @file_name: srt_bandwidth.hpp
* @date: 2022/08/27
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
#include "srt_bandwidth.hpp"
#include <chrono>
uint64_t &bandwidth_mode::bandwidth() {
    return this->band_width;
}

void bandwidth_mode::set_bandwidth(uint64_t t) {
    this->band_width = t;
}

uint64_t bandwidth_mode::get_bandwidth() const {
    return this->band_width;
}

void max_set_bandwidth_mode::input_packet(uint16_t) {}

void constant_rate_mode::input_packet(uint16_t size) {}
void constant_rate_mode::set_bandwidth(uint64_t t) {
    this->bandwidth() = static_cast<uint64_t>(t * 1.25 + 0.5);
}

estimated_bandwidth_mode::estimated_bandwidth_mode() {
    this->bandwidth() = 4 * 1024 * 1024;
}

void estimated_bandwidth_mode::input_packet(uint16_t size) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::lock_guard<std::mutex> lmtx(mtx);
    bytes += size;
    if (now - _last_input_time_point >= 1000 && _last_input_time_point) {
        this->bandwidth() = static_cast<uint64_t>((double) bytes * 1.25);
        bytes = 0;
        _last_input_time_point = now;
    }
    if (!_last_input_time_point) {
        _last_input_time_point = now;
    }
}

uint64_t estimated_bandwidth_mode::get_bandwidth() const {
    std::lock_guard<std::mutex> lmtx(mtx);
    auto v = bandwidth_mode::get_bandwidth();
    return v == 0 ? 1024 * 1024 : v;
}
