/*
* @file_name: packet_sending_queue.hpp
* @date: 2022/09/09
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

#ifndef TOOLKIT_PACKET_SENDING_QUEUE_HPP
#define TOOLKIT_PACKET_SENDING_QUEUE_HPP
#include "event_poller.hpp"
#include "net/asio.hpp"
#include "packet_interface.hpp"
#include "spdlog/logger.hpp"
#include "srt_ack.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <numeric>
#include <utility>
#include <vector>
template<typename T>
class packet_sending_queue : public packet_send_interface<T>, public std::enable_shared_from_this<packet_sending_queue<T>> {
public:
    using packet_pointer = typename packet_interface<T>::packet_pointer;

private:
    using iterator = typename std::list<packet_pointer>::iterator;
    using time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;

public:
    packet_sending_queue(const event_poller::Ptr &poller, const std::shared_ptr<srt::srt_ack_queue> &ack_queue, bool enable_nak = true, bool enable_drop = true)
        : poller(poller), _ack_queue(ack_queue), enable_nak(enable_nak), enable_drop(enable_drop), rexmit_timer(poller->get_executor()) {}

    void set_current_sequence(uint32_t seq) override {
        this->cur_seq = seq;
    }

    uint32_t get_current_sequence() const override {
        return cur_seq;
    }

    uint32_t get_buffer_size() const override {
        return _size.load(std::memory_order_relaxed);
    }

    packet_pointer get_first_block() const override {
        if (_size.load(std::memory_order_relaxed) == 0) {
            return nullptr;
        }
        for (int i = (int) _pkt_cache.size() - 1; i >= 0; i--) {
            if (_pkt_cache[i]->empty()) {
                continue;
            }
            return _pkt_cache[i]->front();
        }
        return nullptr;
    }

    packet_pointer get_last_block() const override {
        if (_size.load(std::memory_order_relaxed) == 0) {
            return nullptr;
        }
        for (int i = 0; i < (int) _pkt_cache.size(); i++) {
            if (_pkt_cache[i]->empty()) {
                continue;
            }
            return _pkt_cache[i]->back();
        }
        return nullptr;
    }

    int input_packet(const T &t, uint32_t seq, uint64_t time_point) override {
        Trace("current seq={}, time_point={}", seq, time_point);
        auto pkt = insert_packet(t, this->get_next_sequence());
        on_packet(pkt);
        return static_cast<int>(t->size());
    }

    void drop(uint32_t seq_begin, uint32_t seq_end) override {
        if (_size.load(std::memory_order_relaxed) == 0 || _pkt_cache.empty()) {
            Debug("no packet to drop, cache is empty");
            return;
        }
        for (int i = 0; i < (int) _pkt_cache.size(); i++) {
            drop_l(i, seq_begin, seq_end);
        }
    }

    void clear() override {
        rexmit_timer.cancel();
        _pkt_cache.clear();
        _allocated_bytes = 0;
        _size.store(0);
    }

    void send_again(uint32_t begin, uint32_t end) override {
        for (int i = (int) (_pkt_cache.size() - 1); i >= 0; i--) {
            send_again_l(i, begin, end);
        }
        /// 更新定时器
        if (enable_drop)
            update_rexmit_timer();
    }

    void ack_sequence_to(uint32_t seq) override {
        if (_size.load(std::memory_order_relaxed) == 0) {
            Trace("no packets need sequence to do, cache is empty");
            return;
        }

        if (packet_interface<T>::is_cycle() && get_last_block()->seq < seq) {
            Trace("out of window range size");
            return;
        }

        auto old = _size.load(std::memory_order_relaxed);
        for (int i = (int) _pkt_cache.size() - 1; i >= 0; i--) {
            ack_sequence_to_l(i, seq);
        }
        auto new_ = _size.load(std::memory_order_relaxed);
        if (old != new_) {
            on_size_changed(false, _size.load());
            if (enable_drop) {
                update_rexmit_timer();
            }
        }
    }

    event_poller::Ptr get_poller() {
        return poller;
    }

protected:
    inline uint32_t get_next_sequence() {
        auto seq = cur_seq;
        cur_seq = (cur_seq + 1) % packet_interface<T>::get_max_sequence();
        return seq;
    }


    packet_pointer insert_packet(const T &t, uint32_t seq, uint64_t submit_time = 0) {
        Trace("current capacity={}, seq={}", packet_interface<T>::capacity(), seq);
        if (packet_interface<T>::capacity() <= 0) {
            auto entry = get_minimum_expired();
            if (entry.first) {
                auto pkt = *(_pkt_cache[entry.second]->begin());
                _pkt_cache[entry.second]->pop_front();
                _size.fetch_sub(1);
                _allocated_bytes -= pkt->pkt->size();
                on_drop_packet(pkt->seq, pkt->seq);
            }
        }
        auto pkt = std::make_shared<packet<T>>();
        pkt->seq = seq;
        if (submit_time == 0)
            pkt->submit_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        else
            pkt->submit_time = submit_time;
        pkt->pkt = t;
        /// 设置重传的的时间点
        pkt->retransmit_time_point = submit_time + pkt_RTO(1) / 1000;
        /// 增加总字节数目
        _allocated_bytes += pkt->pkt->size();

        _size.fetch_add(1);

        if (_pkt_cache.empty()) {
            _pkt_cache.emplace_back(std::make_shared<std::list<packet_pointer>>());
        }
        (*_pkt_cache.begin())->emplace_back(pkt);
        /// 启动定时器
        if (_size.load(std::memory_order_relaxed) == 1 && enable_drop) {
            update_rexmit_timer();
        }
        return pkt;
    }

    void on_packet(const packet_pointer &p) override {
        return packet_interface<T>::_on_packet_func_(p);
    }

    void on_drop_packet(uint32_t begin, uint32_t end) override {
        return packet_interface<T>::_on_drop_packet_func_(begin, end);
    }

    uint64_t get_allocated_bytes() override {
        return this->_allocated_bytes;
    }

    void on_size_changed(bool, uint32_t) override {}
    void update_flow_window(uint32_t) override {}

    /// 更新定时器
    void update_rexmit_timer() {
        /// 拿到最小的超时
        auto entry = get_minimum_expired();
        if (!entry.first) {
            Warn("stop timer, because no pending packets in cache");
            return;
        }
        /// 对应的index桶
        auto index = entry.second;
        const auto &pkt = *(_pkt_cache[index]->begin());
        auto seq = pkt->seq;
        /// 当前这个包的重传时间 us
        auto next_expired = pkt->retransmit_time_point;
        Trace("seq {} set retransmit packet time={} ms", pkt->seq, next_expired);
        std::weak_ptr<packet_sending_queue<T>> self(packet_sending_queue<T>::shared_from_this());
        rexmit_timer.expires_at(time_point(next_expired));
        rexmit_timer.async_wait([self, index, seq, next_expired](const std::error_code &e) {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            if (e) return;
            stronger_self->on_timer(index, seq, next_expired);
        });
    }

    void on_timer(const uint32_t index, const uint32_t seq, uint64_t retransmit_time) {
        Debug("timer expired, index={}, seq={}, time_point={}", index, seq, retransmit_time);
        /// 找到对应桶
        auto _list = _pkt_cache[index];
        /// 需要重传的包
        typename std::list<packet_pointer>::iterator it = _list->begin();
        /// 遍历对应的桶
        bool drop = false;
        auto latency = this->get_max_delay();
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        while (it != _list->end()) {
            /// 丢弃包
            if (now - (*it)->submit_time >= latency && latency) {
                _size.fetch_sub(1);
                _allocated_bytes -= (*it)->pkt->size();
                on_drop_packet((*it)->seq, (*it)->seq);
                it = _list->erase(it);
                drop = true;
                continue;
            }
            /// 如果还没到重传时间点
            if ((*it)->retransmit_time_point > retransmit_time) {
                break;
            }
            /// 重传数+ 1
            ++(*it)->retransmit_count;
            /// 直接重传包
            rexmit_packet((*it));
            /// 更新RTO超时时间
            (*it)->retransmit_time_point += pkt_RTO((*it)->retransmit_count) / 1000;
            ++it;
        }
        /// 放入到下一个桶
        if (it != _list->begin()) {
            /// 如果桶不够, 再添加一个桶
            if ((uint32_t) _pkt_cache.size() < index + 2) {
                _pkt_cache.emplace_back(std::make_shared<std::list<packet_pointer>>());
            }
            /// 添加到下一个桶的末尾
            _pkt_cache[index + 1]->splice(_pkt_cache[index + 1]->end(), *_list, _list->begin(), it);
        }
        /// 更新定时器
        if (enable_drop)
            update_rexmit_timer();
        if (drop) {
            on_size_changed(false, _size.load(std::memory_order_relaxed));
        }
    }

    virtual void rexmit_packet(const packet_pointer &p) {
        if (!enable_nak) {
            return on_packet(p);
        }
    }

    std::pair<iterator, iterator> find_packet_by_sequence(std::list<packet_pointer> &target, uint32_t begin, uint32_t end) {
        auto begin_it = target.begin();
        auto end_it = target.end();
        uint32_t index = 0;
        uint32_t cursor_begin = 0, cursor_end = 0;
        typename std::list<packet_pointer>::iterator seq_begin_it = target.end(), seq_end_it = target.end();

        for (; begin_it != end_it; ++begin_it) {
            ++index;
            if ((*begin_it)->seq == begin) {
                seq_begin_it = begin_it;
                cursor_begin = index;
            }
            if ((*begin_it)->seq == end) {
                seq_end_it = begin_it;
                cursor_end = index;
                break;
            }
        }
        if (seq_begin_it == target.end() || seq_end_it == target.end()) {
            return {target.end(), target.end()};
        }

        if (cursor_begin > cursor_end) {
            std::swap(seq_begin_it, seq_end_it);
        }
        return {seq_begin_it, seq_end_it};
    }

private:
    void drop_l(int index, uint32_t seq_begin, uint32_t seq_end) {
        auto &_list = _pkt_cache[index];
        auto pair = find_packet_by_sequence(*_list, seq_begin, seq_end);
        if (pair.first == _list->end() || pair.second == _list->end()) {
            Debug("no packet to drop, not found begin or end sequence");
            return;
        }
        ++pair.second;
        uint32_t couple = 0;
        std::for_each(pair.first, pair.second, [&](const packet_pointer &p) {
            _allocated_bytes -= p->pkt->size();
            ++couple;
        });
        _list->erase(pair.first, pair.second);
        _size.fetch_sub(couple);
        on_size_changed(false, (uint32_t) _size.load(std::memory_order_relaxed));
        Trace("after drop, packet cache have size={}", _size.load(std::memory_order_relaxed));
    }

    void send_again_l(int index, uint32_t seq_begin, uint32_t seq_end) {
        auto &_list = _pkt_cache[index];
        auto pair = find_packet_by_sequence(*_list, seq_begin, seq_end);
        if (pair.first == _list->end() || pair.second == _list->end()) {
            return;
        }
        ++pair.second;
        Debug("send again {}-{}", seq_begin, seq_end);
        while (pair.first != pair.second) {
            /// 设置为重传
            ++((*pair.first)->retransmit_count);
            on_packet(*pair.first);
            ++pair.first;
        }
        if ((int) _pkt_cache.size() < index + 2) {
            _pkt_cache.emplace_back(std::make_shared<std::list<packet_pointer>>());
        }
        /// 直接移动
        _pkt_cache[index + 1]->splice(_pkt_cache[index + 1]->end(), *_list, pair.first, pair.second);
    }

    void ack_sequence_to_l(int index, uint32_t seq) {
        auto _list = _pkt_cache[index];
        while (!_list->empty()) {
            auto element = _list->front();
            if (element->seq >= seq && !packet_send_interface<T>::is_cycle()) {
                break;
            }
            _allocated_bytes -= element->pkt->size();
            _list->pop_front();
            _size.fetch_sub(1);
        }
    }

    //// 求出最小超时的桶的位置
    std::pair<bool, uint32_t> get_minimum_expired() const {
        if (_size.load(std::memory_order_relaxed) == 0) {
            return {false, 0};
        }
        int _ = (int) _pkt_cache.size();
        /// 遍历桶
        int index = -1;
        uint64_t mini = (std::numeric_limits<uint64_t>::max)();
        for (int i = 0; i < _; i++) {
            if (_pkt_cache[i]->empty()) {
                continue;
            }
            /// 得到第一个的包
            auto &pkt = *_pkt_cache[i]->begin();
            /// 重传的时间点
            auto rexmit = pkt->retransmit_time_point;
            if (mini > rexmit) {
                index = i;
                mini = rexmit;
            }
        }
        if (index == -1) {
            return {false, 0};
        }
        return {true, index};
    }

    uint32_t pkt_RTO(uint32_t counts) const {
        auto rto = _ack_queue->get_rto();
        auto rtt_var = _ack_queue->get_rtt_var();
        auto RTO = counts * (rto + 4 * rtt_var + 20000) + 10000;
        return (uint32_t) (RTO);
    }

private:
    event_poller::Ptr poller;
    std::atomic<uint32_t> _size{0};
    asio::steady_timer rexmit_timer;
    uint32_t cur_seq = 0;
    ///表示有多少个重传桶
    std::vector<std::shared_ptr<std::list<packet_pointer>>> _pkt_cache;
    std::shared_ptr<srt::srt_ack_queue> _ack_queue;
    uint64_t _allocated_bytes = 0;
    bool enable_nak = true;
    bool enable_drop = true;
};


#endif//TOOLKIT_PACKET_SENDING_QUEUE_HPP
