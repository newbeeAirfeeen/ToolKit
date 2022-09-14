/*
* @file_name: srt_congestion.hpp
* @date: 2022/09/13
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

#ifndef TOOLKIT_SRT_CONGESTION_HPP
#define TOOLKIT_SRT_CONGESTION_HPP
#include <atomic>
#include <chrono>

class congestion_holder {
public:
    virtual ~congestion_holder() = default;
    virtual uint32_t get_current_seq() const = 0;
    virtual uint32_t get_RTT() const = 0;
    virtual uint32_t get_ack_last_number() const = 0;
    virtual uint32_t get_lost_list_size() const = 0;
    virtual uint32_t get_max_window_size() const = 0;
    virtual uint32_t get_max_payload() const = 0;
    virtual uint32_t get_deliver_rate() const = 0;
};

class congestion {
private:
    using duration = std::chrono::microseconds;
    using time_point = std::chrono::steady_clock::time_point;

public:
    explicit congestion(const congestion_holder &holder);
    virtual ~congestion() = default;
    /// 已经发送的包数量
    double get_send_period() const;
    uint32_t get_cwnd_window() const;

public:
    virtual bool slow_starting() const;
    virtual void rexmit_pkt_event(bool is_nak, uint32_t begin, uint32_t end);
    virtual void ack_sequence_to(uint32_t seq, uint32_t receive_rate, uint32_t link_capacity);

private:
    void update_pkt_send_period();

private:
    const congestion_holder &holder;
    /// kept at 1 microsecond in order to send packets as fast as possible
    double _pkt_send_period = 1.0;
    /// CWND_SIZE has an upper threshold, which is the maximum allowed congestion window size (MAX_CWND_SIZE)
    std::atomic<uint32_t> cwnd_size{16};
    //// us
    ///  sending rate was either increased or kept(us)
    uint64_t _last_rc_time = 0;
    const uint16_t _rc_internal = 10000;
    std::atomic<bool> in_slow_start{true};
    /// bloss flag is equal to true if a packet loss has happend since the
    /// last sending rate increase.initial value: false;
    bool bloss = false;
    uint32_t last_dec_seq = 0;
    /// The initial value of LastDecPeriod is set to 1 microseconds
    double last_dec_period = 1.0;
    uint32_t _link_capacity = 0;
    /// NAK counter
    uint32_t nak_count = 1;
    /// random threshold on decrease by number of loss events
    uint32_t dec_random = 1;
    /// average number of NAKs per congestion
    uint32_t avg_nak_num = 0;
    /// number of decreases in a congestion epoch
    uint32_t dec_count = 0;
};
#endif//TOOLKIT_SRT_CONGESTION_HPP
