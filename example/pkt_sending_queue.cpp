//
// Created by 沈昊 on 2022/9/10.
//
#include "Util/semaphore.h"
#include "net/event_poller_pool.hpp"
#include "protocol/srt/packet_sending_queue.hpp"
#include "spdlog/logger.hpp"
#include "protocol/srt/srt_ack.hpp"
#ifdef ENABLE_PREF_TOOL
#include <gperftools//profiler.h>
#endif
using namespace srt;
auto create_queue(uint32_t seq, uint32_t delay, uint32_t max_seq, uint32_t window_size, bool enable_drop) -> std::shared_ptr<packet_sending_queue<std::shared_ptr<std::string>>> {
    auto poller = event_poller_pool::Instance().get_poller(false);
    auto ack_queue_ = std::make_shared<srt_ack_queue>();
    auto queue = std::make_shared<packet_sending_queue<std::shared_ptr<std::string>>>(poller, ack_queue_, true, enable_drop);
    ack_queue_->set_rtt(10000, 5000);
    queue->set_current_sequence(seq);
    queue->set_max_delay(delay);
    queue->set_max_sequence(max_seq);
    queue->set_window_size(window_size);
    queue->set_on_drop_packet([](uint32_t begin, uint32_t end) {
        Info("\tdrop packet {}-{}", begin, end);
    });
    queue->set_on_packet([](const std::shared_ptr<packet<std::shared_ptr<std::string>>> &pointer) {
        if(pointer->retransmit_count > 1){
            Warn("rexmit packet, seq={}, data={}", pointer->seq, pointer->pkt->data());
        }
        else{
            Info("send pkt, seq={}, data={}", pointer->seq, pointer->pkt->data());
        }
    });
    return queue;
}
int main() {
    const char *exec_name = "pkt_sending_queue.profile";
    logger::initialize("logs/pkt_sending_queue.log", spdlog::level::debug);
#ifdef ENABLE_PREF_TOOL
    ProfilerStart(exec_name);
#endif
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });
    auto str = std::make_shared<std::string>("0");
    auto queue = create_queue(16, 1020, 32, 8, true);
    std::condition_variable cv;
    std::mutex mtx;
    auto poller = queue->get_poller();

    //// 构建定时器模拟发包
    auto send_timer = poller->do_delay_task(std::chrono::milliseconds(50), [&]() -> size_t {
        Info("\t\tsend pkt, seq={}", queue->get_current_sequence());
        queue->input_packet(str, 0, 0);//16
        return 50;
    });


    auto nak_timer = poller->do_delay_task(std::chrono::milliseconds(100), [&]()->size_t {
        auto seq = queue->get_current_sequence();
        if( seq > 10){
            queue->send_again(queue->get_current_sequence() - 6, queue->get_current_sequence() - 4);
        }
        return 100;
    });



    sem.wait();
#ifdef ENABLE_PREF_TOOL
    ProfilerStop();
#endif
    return 0;
}