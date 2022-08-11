/*
* @file_name: srt_socket_service.cpp
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
#include "srt_socket_base.hpp"
#include "srt_error.hpp"
#include <system_error>
namespace srt {

    void srt_socket_base::set_max_payload(uint32_t length) {
        if (is_open()) {
            return;
        }
        if (length > 1500) {
            throw std::system_error(make_srt_error(srt_error_code::too_large_payload));
        }
        this->max_payload = length;
    }

    void srt_socket_base::set_max_flow_window_size(uint32_t counts) {
        if (is_open()) {
            return;
        }
        this->max_flow_window_size = counts;
    }

    void srt_socket_base::set_drop_too_late_packet(bool on) {
        if (is_open()) {
            return;
        }
        this->drop_too_late_packet = on;
    }

    void srt_socket_base::set_time_based_deliver(uint64_t ms) {
        if (is_open()) {
            return;
        }
        this->time_deliver_ = ms;
    }

    void srt_socket_base::set_report_nak(bool on) {
        if (is_open()) {
            return;
        }
        this->report_nak = on;
    }

    void srt_socket_base::set_stream_id(const std::string &id) {
        if (is_open()) {
            return;
        }
        this->stream_id = id;
    }

    void srt_socket_base::set_sock_id(uint32_t id) {
        if (is_open()) {
            return;
        }
        this->sock_id = id;
    }

    void srt_socket_base::set_connect_timeout(uint64_t ms) {
        if (is_open()) {
            return;
        }
        this->connect_time_out = ms;
    }

    void srt_socket_base::set_max_receive_time_out(uint32_t ms) {
        if (is_open()) {
            return;
        }
        this->max_receive_time_out = ms;
    }

    uint32_t srt_socket_base::get_max_payload() const {
        return this->max_payload;
    }

    uint32_t srt_socket_base::get_max_flow_window_size() const {
        return this->max_flow_window_size;
    }

    bool srt_socket_base::get_drop_too_late_packet() const {
        return this->drop_too_late_packet;
    }

    uint32_t srt_socket_base::get_time_based_deliver() const {
        return this->time_deliver_;
    }

    uint32_t srt_socket_base::get_sock_id() const {
        return this->sock_id;
    }

    bool srt_socket_base::get_report_nak() const {
        return this->report_nak;
    }

    const std::string &srt_socket_base::get_stream_id() const {
        return this->stream_id;
    }

    uint32_t srt_socket_base::get_connect_timeout() const {
        return this->connect_time_out;
    }

    uint32_t srt_socket_base::get_max_receive_time_out() const {
        return this->max_receive_time_out;
    }
};// namespace srt
