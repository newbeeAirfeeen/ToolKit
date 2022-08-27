/*
* @file_name: srt_ack.hpp
* @date: 2022/08/23
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

#ifndef TOOLKIT_SRT_ACK_HPP
#define TOOLKIT_SRT_ACK_HPP
#include <chrono>
#include <map>
#include <unordered_map>
namespace srt {


    class packet_receive_rate {
    public:
        explicit packet_receive_rate(const std::chrono::steady_clock::time_point &);
        void input_packet(const std::chrono::steady_clock::time_point &);
        uint32_t get_packet_receive_rate();

    private:
        std::chrono::steady_clock::time_point _start;
        std::map<int64_t, int64_t> _pkt_map;
    };


    class estimated_link_capacity {
    public:
        explicit estimated_link_capacity(const std::chrono::steady_clock::time_point &);
        void input_packet(const std::chrono::steady_clock::time_point &);
        uint32_t get_estimated_link_capacity();

    private:
        std::chrono::steady_clock::time_point _start;
        std::map<int64_t, int64_t> _pkt_map;
    };

    class receive_rate {
    public:
        explicit receive_rate(const std::chrono::steady_clock::time_point &);
        void input_packet(const std::chrono::steady_clock::time_point &, size_t size);
        uint32_t get_receive_rate();

    private:
        std::chrono::steady_clock::time_point _start;
        std::map<int64_t, int64_t> _pkt_map;
    };


    class srt_ack_queue {
    public:
        void set_rtt(double _rtt, double _rtt_var);
        void add_ack(uint32_t);
        void calculate(uint32_t);
        uint32_t get_rto() const;
        uint32_t get_rtt_var() const;

    private:
        /// 单位是微秒
        double _rtt = 100000;
        double _rtt_var = 50000;
        std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> ack_queue;
    };
}// namespace srt


#endif//TOOLKIT_SRT_ACK_HPP
