/*
* @file_name: packet_limited_send_queue.hpp
* @date: 2022/08/27
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

#ifndef TOOLKIT_PACKET_LIMITED_SEND_QUEUE_HPP
#define TOOLKIT_PACKET_LIMITED_SEND_QUEUE_HPP
#include "deadline_timer.hpp"
#include "packet_send_queue.hpp"
#include "spdlog/logger.hpp"
#include <map>
template<typename T>
class packet_limited_send_queue : public packet_send_queue<T>, public std::enable_shared_from_this<packet_limited_send_queue<T>> {
public:
    using packet_pointer = typename packet_send_queue<T>::packet_pointer;

private:
    using base_type = std::enable_shared_from_this<packet_limited_send_queue<T>>;
    using duration_type = std::chrono::microseconds;

public:
    explicit packet_limited_send_queue(asio::io_context &io_context, uint16_t payload = 1456) {
        Trace("create limited send queue, payload={}", payload);
        timer = create_deadline_timer<packet_pointer, duration_type>(io_context);
        avg_payload_size = payload > 1456 ? 1472 : (payload + 16);
        Trace("average payload size={}", avg_payload_size);
        update_snd_period();
    }
    ~packet_limited_send_queue() override = default;

public:
    void start() override {
        std::weak_ptr<packet_limited_send_queue<T>> self(base_type::shared_from_this());
        timer->set_on_expired([self](const packet_pointer &pkt) {
            auto stronger_self = self.lock();
            if (!stronger_self) { return; }
            return stronger_self->on_timer(pkt);
        });
    }
    ///(1) On sending a data packet (either original or retransmitted),
    /// update the value of average packet payload size (AvgPayloadSize):
    void on_packet(const packet_pointer &p) override {
        update_avg_payload(static_cast<uint16_t>(p->pkt->size()));
        /// 更新为上一次发送数据包的seq
        return packet_send_queue<T>::on_packet(p);
    }

    void ack_sequence_to(uint32_t seq) override {
        update_snd_period();
        return packet_send_queue<T>::ack_sequence_to(seq);
    }

private:
    void on_timer(const packet_pointer &pkt) {
        return packet_send_queue<T>::on_packet(pkt);
    }

private:
    inline void update_avg_payload(uint16_t size) {
        avg_payload_size = static_cast<uint16_t>(7.0 / 8 * avg_payload_size + 1.0 / 8 * size);
        Trace("update average payload size={}", avg_payload_size);
    }

    inline void update_snd_period() {
        /// 求出步长

        //_pkt_snd_period = avg_payload_size * 1e6 / packet_send_queue<T>::get_bandwidth_mode()->get_bandwidth() * 1.0;
        Trace("update packet send period={} us", static_cast<uint64_t>(_pkt_snd_period));
    }

private:
    /// AvgPayloadSize is equal to the maximum
    /// allowed packet payload size, which cannot be larger than 1456 bytes.
    /// AvgPayloadSize = 7/8 * AvgPayloadSize + 1/8 * PacketPayloadSize
    uint16_t avg_payload_size = 1472;
    /// PKT_SND_PERIOD = PktSize * 1000000 / MAX_BW
    ///  microseconds
    double _pkt_snd_period = 11.776;
    std::shared_ptr<deadline_timer<packet_pointer, duration_type>> timer;
};


#endif//TOOLKIT_PACKET_LIMITED_SEND_QUEUE_HPP
