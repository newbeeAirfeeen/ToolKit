﻿#include "srt_ack.hpp"
#include "spdlog/logger.hpp"
#include <algorithm>
#include <vector>
namespace srt {

    packet_receive_rate::packet_receive_rate(const std::chrono::steady_clock::time_point &t) : _start(t) {}
    void packet_receive_rate::input_packet(const std::chrono::steady_clock::time_point &t) {
        if (_pkt_map.size() > 100) {
            _pkt_map.erase(_pkt_map.begin());
        }
        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(t - _start).count();
        _pkt_map.emplace(ts, ts);
    }

    uint32_t packet_receive_rate::get_packet_receive_rate() {
        if (_pkt_map.size() < 2) {
            return 50000;
        }
        int64_t dur = 1000;
        for (auto it = _pkt_map.begin(); it != _pkt_map.end(); ++it) {
            auto next = it;
            ++next;
            if (next == _pkt_map.end()) {
                break;
            }

            if ((next->first - it->first) < dur) {
                dur = next->first - it->first;
            }
        }

        double rate = 1e6 / (double) dur;
        if (rate <= 1000) {
            return 50000;
        }
        return (uint32_t) rate;
    }

    estimated_link_capacity::estimated_link_capacity(const std::chrono::steady_clock::time_point &t) : _start(t) {}

    void estimated_link_capacity::input_packet(const std::chrono::steady_clock::time_point &t) {
        if (_pkt_map.size() > 16) {
            _pkt_map.erase(_pkt_map.begin());
        }
        auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(t - _start).count();
        _pkt_map.emplace(tmp, tmp);
    }

    uint32_t estimated_link_capacity::get_estimated_link_capacity() {
        decltype(_pkt_map.begin()) next;
        std::vector<int64_t> tmp;

        for (auto it = _pkt_map.begin(); it != _pkt_map.end(); ++it) {
            next = it;
            ++next;
            if (next != _pkt_map.end()) {
                tmp.push_back(next->first - it->first);
            } else {
                break;
            }
        }
        std::sort(tmp.begin(), tmp.end());
        if (tmp.empty()) {
            return 1000;
        }

        if (tmp.size() < 16) {
            return 1000;
        }

        double dur = tmp[0] / 1e6;
        return (uint32_t) (1.0 / dur);
    }

    receive_rate::receive_rate(const std::chrono::steady_clock::time_point &t) : _start(t) {}
    void receive_rate::input_packet(const std::chrono::steady_clock::time_point &t, size_t size) {
        if (_pkt_map.size() > 100) {
            _pkt_map.erase(_pkt_map.begin());
        }
        auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(t - _start).count();
        _pkt_map.emplace(tmp, tmp);
    }

    uint32_t receive_rate::get_receive_rate() {
        if (_pkt_map.size() < 2) {
            return 0;
        }

        auto first = _pkt_map.begin();
        auto last = _pkt_map.rbegin();
        double dur = (double) (last->first - first->first) / 1000000.0;

        size_t bytes = 0;
        for (auto it: _pkt_map) {
            bytes += it.second;
        }
        double rate = (double) bytes / dur;
        return (uint32_t) rate;
    }


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