#include "srt_ack.hpp"
namespace srt {
    void srt_ack_queue::set_rtt(double _rtt, double _rtt_var) {
        this->_rtt = _rtt;
        this->_rtt_var = _rtt_var;
    }

    void srt_ack_queue::add_ack(uint32_t seq) {
        ack_queue.emplace(seq, std::chrono::steady_clock::now());
    }

    void srt_ack_queue::calculate(uint32_t seq) {
        auto it = ack_queue.find(seq);
        if (it == ack_queue.end()) {
            return;
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - it->second).count();
        /// rtt = 7/8 * RTT + 1/8 * rtt
        _rtt = 7 / 8 * ms + 1 / 8 * _rtt;
        /// rtt_var = 3/4 * rtt_var + 1/4 * abs(RTT - rtt)
        _rtt_var = 3 / 4 * _rtt_var + 1 / 4 * std::abs(ms - _rtt);
        ack_queue.erase(it);
    }


    uint32_t srt_ack_queue::get_rto() const {
        return static_cast<uint32_t>(this->_rtt);
    }

    uint32_t srt_ack_queue::get_rtt_var() const {
        return static_cast<uint32_t>(this->_rtt_var);
    }

}// namespace srt