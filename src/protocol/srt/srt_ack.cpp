#include "srt_ack.hpp"
#include "spdlog/logger.hpp"
#include <algorithm>
#include <vector>
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
        auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - it->second).count();
        /// rtt = 7/8 * RTT + 1/8 * rtt
        /// rtt_var = 3/4 * rtt_var + 1/4 * abs(RTT - rtt)
        _rtt_var = (3 * _rtt_var + std::abs((long) _rtt - (long) rtt)) / 4;
        _rtt = (7 * rtt + _rtt) / 8;
        Debug("rtt={}, rtt_variance={}, ms={}", _rtt, _rtt_var, rtt);
        ack_queue.erase(it);
    }


    uint32_t srt_ack_queue::get_rto() const {
        return this->_rtt;
    }

    uint32_t srt_ack_queue::get_rtt_var() const {
        return this->_rtt_var;
    }

}// namespace srt