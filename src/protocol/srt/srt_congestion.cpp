/*
* @file_name: srt_congestion.cpp
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
#include "srt_congestion.hpp"
#include "spdlog/logger.hpp"
#include <algorithm>
#include <cmath>
#include <random>
static std::mt19937 &random_gen() {
    static thread_local std::random_device ran;
    static thread_local std::mt19937 sgen(ran());
    return sgen;
}

static int get_random_int(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(random_gen());
}

inline static int seq_cmp(int32_t seq, int32_t seq2) {
    constexpr int32_t max_seq = 0x3FFFFFFF;
    return (std::abs(seq - seq2) < max_seq) ? (seq - seq2) : (seq2 - seq);
}

inline static uint32_t seq_len(uint32_t seq1, uint32_t seq2) {
    return (seq1 <= seq2) ? (seq2 - seq1 + 1) : (seq2 - seq1 + 0x7FFFFFFF + 2);
}
inline static int32_t dec_seq(int32_t seq) { return (seq == 0) ? 0x7FFFFFFF : seq - 1; }


congestion::congestion(const congestion_holder &holder) : holder(holder) {
    _last_rc_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    last_dec_seq = (uint32_t) dec_seq((int32_t) holder.get_ack_last_number());
}

double congestion::get_send_period() const {
    return _pkt_send_period;
}

uint32_t congestion::get_cwnd_window() const {
    return this->cwnd_size.load(std::memory_order_relaxed);
}

bool congestion::slow_starting() const {
    return in_slow_start.load(std::memory_order_relaxed);
}

void congestion::rexmit_pkt_event(bool is_nak, uint32_t begin, uint32_t end) {
    /// slow start phase ends
    /// set the pkt sending period pkt_snd_period  in step 5 of section(1)

    /// on a retransmission timeout(RTO) event.
    if (in_slow_start.load()) {
        Debug("stop to slow starting...");
        in_slow_start.store(false);
        update_pkt_send_period();
    }
    if (!is_nak) {
        return;
    }

    bloss = true;
    const auto pkts_in_flight = static_cast<uint32_t>(holder.get_RTT() / _pkt_send_period);
    const auto lost_pcent_x10 = pkts_in_flight > 0 ? (holder.get_lost_list_size() * 1000) / pkts_in_flight : 0;
    if (lost_pcent_x10 < 20) {
        last_dec_period = _pkt_send_period;
        return;
    }

    if (seq_cmp((int32_t) begin, (int32_t) last_dec_seq) > 0) {
        last_dec_period = _pkt_send_period;
        _pkt_send_period = std::ceil(_pkt_send_period * 1.03);
        constexpr const double loss_share_factor = 0.03;
        avg_nak_num = (uint32_t) ceil(avg_nak_num * (1 - loss_share_factor) + nak_count * loss_share_factor);
        nak_count = dec_count = 1;
        last_dec_seq = holder.get_current_seq();
        dec_random = avg_nak_num > 1 ? get_random_int(1, (int) avg_nak_num) : 1;
    } else if ((dec_count++ < 5) && (0 == (++nak_count % dec_random))) {
        _pkt_send_period = std::ceil(_pkt_send_period * 1.03);
        last_dec_seq = holder.get_current_seq();
    }
}

void congestion::ack_sequence_to(uint32_t seq, uint32_t receive_rate, uint32_t link_capacity) {
    auto now = (uint64_t) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (now - _last_rc_time < _rc_internal) {
        return;
    }

    _last_rc_time = now;
    if (in_slow_start.load()) {
        /// CWND_SIZE += ACK_SEQNO - LAST_ACK_SEQNO
        /// LAST_ACK_SEQNO = ACK_SEQNO
        /// Step 3.
        cwnd_size.fetch_add(seq_len(holder.get_ack_last_number(), seq));
        /// step 5
        if (cwnd_size.load(std::memory_order_relaxed) >= holder.get_max_window_size()) {
            update_pkt_send_period();
            in_slow_start.store(false);
            Debug("stop to slow staring...");
        }
    }
    /// once slow start phase ends , the algorithm enters the congestion avoidance phase
    else {
        cwnd_size.store(holder.get_deliver_rate() * (holder.get_RTT() + _rc_internal) / 1000000 + 16);
        Trace("update cwnd size={}", cwnd_size.load(std::memory_order_relaxed));
        if (bloss) {
            bloss = false;
        } else {
            double inc = 0;
            const auto loss_bandwidth = static_cast<uint32_t>(2.0 * (1000000.0 / last_dec_period));
            this->_link_capacity = std::min(loss_bandwidth, link_capacity);
            auto _max_mss = holder.get_max_payload();
            auto B = (uint64_t) (this->_link_capacity - 1000000.0 / _pkt_send_period);
            if (_pkt_send_period > last_dec_period && ((_link_capacity / 9) < B)) {
                B = _link_capacity / 9;
            }
            if (B <= 0)
                inc = 1.0 / _max_mss;
            else {
                inc = std::pow(10.0, std::ceil(std::log10(B * _max_mss * 8.0))) * 0.0000015 / _max_mss;
                inc = std::max(inc, 1.0 / _max_mss);
            }
            _pkt_send_period = (_pkt_send_period * _rc_internal) / (_pkt_send_period * inc + _rc_internal);
            Trace("update packet send period={} ns", (uint64_t) (_pkt_send_period * 1000));
        }
    }
}

void congestion::update_pkt_send_period() {
    auto deliver_rate = holder.get_deliver_rate();
    if (deliver_rate > 0) {
        _pkt_send_period = 1000000.0 / deliver_rate;
    } else {
        _pkt_send_period = cwnd_size.load(std::memory_order_relaxed) * 1.0 / (holder.get_RTT() + _rc_internal);
    }
    Trace("update packet send period={} ns", (uint64_t) (_pkt_send_period * 1000));
}