/*
* @file_name: socket_statistic.hpp
* @date: 2022/08/08
* @author: oaho
* Copyright @ hz oaho, All rights reserved.
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

#ifndef TOOLKIT_SOCKET_STATISTIC_HPP
#define TOOLKIT_SOCKET_STATISTIC_HPP
#include "deadline_timer.hpp"
#include "net/asio.hpp"
#include <cstdint>
#include <list>
#include <mutex>

class socket_statistic : public std::enable_shared_from_this<socket_statistic> {
public:
    virtual ~socket_statistic() = default;

public:
    explicit socket_statistic(asio::io_context &poller);
    /// bytes statistic
    void delta_bytes(uint64_t);
    double get_bytes_rate() const;
    /// packet statistic
    void report_packet(uint64_t count);
    void report_packet_lost(uint64_t count);
    uint64_t get_packet_count() const;

public:
    void start();
    void reset();

private:
    void on_timer();

private:
    uint64_t last_bytes = 0;
    uint64_t bytes = 0;

    uint64_t lost_packet_count = 0;
    uint64_t packet_count = 0;

private:
    mutable std::recursive_mutex mtx;
    std::shared_ptr<deadline_timer<int>> timer;
    std::shared_ptr<std::list<uint64_t>> samples;
};


#endif//TOOLKIT_SOCKET_STATISTIC_HPP
