/*
* @file_name: socket_statistic.cpp
* @date: 2022/08/08
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
#include "socket_statistic.hpp"
#include <algorithm>

socket_statistic::socket_statistic(asio::io_context &poller) {
    timer = create_deadline_timer<int>(poller);
    samples = std::make_shared<std::list<uint64_t>>();
}

void socket_statistic::delta_bytes(uint64_t b) {
    bytes += b;
}

double socket_statistic::get_bytes_rate() const {

    decltype(samples) _tmp_samples;
    {
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        _tmp_samples = samples;
    }

    if (!_tmp_samples) {
        return 0;
    }


    if (_tmp_samples->empty()) {
        return 0;
    }

    uint64_t sum = 0;
    std::for_each(_tmp_samples->begin(), _tmp_samples->end(), [&](const uint64_t &item) {
        sum += item;
    });
    return (double) sum / (double) _tmp_samples->size();
}

void socket_statistic::report_packet(uint64_t count) {
    packet_count += count;
}

void socket_statistic::report_packet_lost(uint64_t count) {
    lost_packet_count += count;
}

uint64_t socket_statistic::get_packet_count() const {
    return this->packet_count;
}

void socket_statistic::start() {
    std::weak_ptr<socket_statistic> self(shared_from_this());
    timer->set_on_expired([self](const int &) {
        auto stronger_self = self.lock();
        if (!stronger_self) {
            return;
        }
        stronger_self->on_timer();
        stronger_self->timer->add_expired_from_now(1000, 1);
    });
    timer->add_expired_from_now(1000, 1);
}

void socket_statistic::reset() {
    last_bytes = bytes = lost_packet_count = packet_count = 0;
    {
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        samples->clear();
    }
    timer->stop();
    start();
}

void socket_statistic::on_timer() {
    /// 偏移的字节数
    auto delta_bytes = bytes - last_bytes;
}
