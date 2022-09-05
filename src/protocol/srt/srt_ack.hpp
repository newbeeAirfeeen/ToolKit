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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
namespace srt {

    struct receive_rate {
        uint32_t pkt_per_sec = 0;
        uint32_t bytes_per_sec = 0;
    };

    template<uint16_t NUM = 16, uint16_t PNUM = 64>
    class packet_calculate_window {
    public:
        packet_calculate_window() {
            std::fill(pkt_window, pkt_window + NUM, 1000000);
            std::fill(bytes_window, bytes_window + NUM, 1474);
            std::fill(bandwidth_window, bandwidth_window + PNUM, 1000);
        }

    public:
        void update_receive_rate(uint16_t size) {
            auto now = std::chrono::steady_clock::now();
            pkt_window[index] = (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(now - last_receive_point).count();
            bytes_window[index] = size;
            index = (index + 1) % NUM;
            last_receive_point = now;
        }

        void update_estimated_capacity(uint32_t seq, uint16_t size, bool unordered) {
            auto mod = seq % 16;
            /// 每16个包一次
            if (!mod) {
                estimated_1(seq, unordered);
            }

            if (unordered) {
                return;
            }

            if (mod == 1) {
                estimated_2(seq, size);
            }
        }

        receive_rate get_pkt_receive_rate() const {
            uint32_t tmp_window[NUM] = {0};
            std::copy(pkt_window, pkt_window + NUM, tmp_window);
            auto median_index = NUM >> 1;
            std::nth_element(tmp_window, tmp_window + median_index, tmp_window + NUM);
            auto median = tmp_window[median_index];
            auto upper = median << 3;
            auto lower = median >> 3;

            uint32_t count = 0;
            uint64_t usec_sum = 0;
            uint32_t bytes = 0;

            for (int i = 0; i < NUM; i++) {
                if (pkt_window[i] <= lower || pkt_window[i] >= upper) {
                    continue;
                }
                ++count;
                usec_sum += pkt_window[i];
                bytes += bytes_window[i];
            }

            if (count > (NUM >> 1)) {
                receive_rate r;
                r.pkt_per_sec = static_cast<uint32_t>(std::ceil(1000000.0 / (1.0 * usec_sum / count)));
                r.bytes_per_sec = static_cast<uint32_t>(std::ceil(1000000.0 / usec_sum / bytes));
                return r;
            }
            return {};
        }

        uint32_t get_bandwidth() const {
            uint32_t tmp_bandwidth_window[PNUM]{0};
            auto bindex_middle_index = PNUM >> 1;
            std::copy(bandwidth_window, bandwidth_window + bindex_middle_index, tmp_bandwidth_window);
            std::nth_element(tmp_bandwidth_window, tmp_bandwidth_window + bindex_middle_index, tmp_bandwidth_window + PNUM);
            uint32_t median = tmp_bandwidth_window[bindex_middle_index];

            uint32_t count = 0;
            uint32_t sum = median;
            auto upper = median << 3;// median*8
            auto lower = median >> 3;// median/8
            uint32_t bandsum = 0;
            // median filtering
            for (int i = 0; i < (int) PNUM; ++i) {
                if (bandwidth_window[i] <= lower || bandwidth_window[i] >= upper) {
                    continue;
                }
                ++count;
                bandsum += bandwidth_window[i];
            }
            if (count == 0) count = 1;
            return (uint32_t)(std::ceil(1000000.0 / (double(bandsum) / double(count))));
        }

    private:
        void estimated_1(uint32_t seq, bool unordered) {
            if (unordered && (int64_t) (seq) == sequence_1) {
                sequence_1 = -1;
                return;
            }
            commit_1 = std::chrono::steady_clock::now();
            sequence_1 = seq;
        }

        void estimated_2(uint32_t seq, uint16_t size) {
            auto next_seq = (sequence_1 + 1) % 0x7FFFFFFF;
            if (sequence_1 == -1 || next_seq != seq) {
                return;
            }
            auto now = std::chrono::steady_clock::now();
            auto micro_dur = std::chrono::duration_cast<std::chrono::microseconds>(now - commit_1).count();
            auto pl_size = 1456 * micro_dur;


            commit_1 = now;
            sequence_1 = -1;
            bandwidth_window[bindex] = size ? (uint32_t) (pl_size / size) : (uint32_t) micro_dur;
            bindex = (bindex + 1) % PNUM;
        }

    private:
        uint32_t pkt_window[NUM]{0};
        uint32_t bytes_window[NUM]{0};
        uint16_t index = 0;
        uint32_t bandwidth_window[PNUM]{0};
        uint16_t bindex = 0;
        int64_t sequence_1 = -1;
        std::chrono::steady_clock::time_point commit_1;
        std::chrono::steady_clock::time_point last_receive_point{std::chrono::steady_clock::now()};
    };


    class srt_ack_queue {
    public:
        void set_rtt(uint32_t _rtt, uint32_t _rtt_var);
        void add_ack(uint32_t);
        void calculate(uint32_t);
        uint32_t get_rto() const;
        uint32_t get_rtt_var() const;

    private:
        /// 单位是微秒
        uint32_t _rtt = 100000;
        uint32_t _rtt_var = 50000;
        std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> ack_queue;
    };
}// namespace srt


#endif//TOOLKIT_SRT_ACK_HPP
