/*
* @file_name: srt_socket_service.hpp
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

#ifndef TOOLKIT_SRT_SOCKET_BASE_HPP
#define TOOLKIT_SRT_SOCKET_BASE_HPP
#include <cstdint>
#include <string>
namespace srt {

    class srt_socket_base {
    public:
        virtual ~srt_socket_base() = default;
        /// 连接成功
        virtual bool is_open() = 0;
        virtual bool is_connected() = 0;
    public:
        void set_max_payload(uint32_t length);
        void set_max_flow_window_size(uint32_t counts);
        void set_drop_too_late_packet(bool on);
        void set_time_based_deliver(uint64_t ms);
        void set_report_nak(bool on);
        void set_stream_id(const std::string &stream_id);
        void set_sock_id(uint32_t id);
        void set_connect_timeout(uint64_t ms);
        void set_max_receive_time_out(uint32_t ms);
        uint32_t get_max_payload() const;
        uint32_t get_max_flow_window_size() const;
        bool get_drop_too_late_packet() const;
        uint32_t get_time_based_deliver() const;
        bool get_report_nak() const;
        const std::string &get_stream_id() const;
        uint32_t get_sock_id() const;
        uint32_t get_connect_timeout() const;
        uint32_t get_max_receive_time_out() const;

    protected:
        bool drop_too_late_packet = true;
        bool report_nak = true;
        uint32_t time_deliver_ = 120;
        std::string stream_id;
        uint32_t max_payload = 1500;
        uint32_t max_flow_window_size = 8192;
        uint32_t sock_id = 0;
        uint32_t connect_time_out = 1000;
        uint32_t max_receive_time_out = 10000;
    };

};// namespace srt

#endif//TOOLKIT_SRT_SOCKET_BASE_HPP
