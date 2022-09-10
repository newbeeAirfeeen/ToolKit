//
// Created by 沈昊 on 2022/8/5.
//
#include "net/event_poller_pool.hpp"
#include "protocol/srt/packet_sending_queue.hpp"
#include "protocol/srt/srt_ack.hpp"
#include "spdlog/logger.hpp"
#include <gtest/gtest.h>
using namespace std;
using namespace srt;

#define SLEEP_MS(VAR) std::this_thread::sleep_for(std::chrono::milliseconds(VAR));
#define SLEEP_SECOND(VAR) SLEEP_MS(VAR * 1000)

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
        //Info("drop packet {}-{}", begin, end);
    });
    queue->set_on_packet([](const std::shared_ptr<packet<std::shared_ptr<std::string>>> &pointer) {
        //Info("send pkt: {}", pointer->pkt->data());
    });
    return queue;
}
TEST(cycle_ack_sequence_to, packet_sending_queue) {
    logger::initialize("logs/pkt_sending_queue_add_unittest.log", spdlog::level::debug);
    auto queue = create_queue(16, 500, 20, 8, false);
    auto str = std::make_shared<std::string>("0");
    // 16
    queue->input_packet(str, 0, 0);
    // 17
    queue->input_packet(str, 0, 0);
    // 18
    queue->input_packet(str, 0, 0);
    // 19
    queue->input_packet(str, 0, 0);
    // 0
    queue->input_packet(str, 0, 0);
    // 1
    queue->input_packet(str, 0, 0);
    // 2
    queue->input_packet(str, 0, 0);

    queue->ack_sequence_to(15);
    EXPECT_EQ(queue->get_buffer_size(), 7);
    queue->ack_sequence_to(2);
    EXPECT_EQ(queue->get_buffer_size(), 1);
}
TEST(basic_ack_sequence_to, packet_sending_queue) {
    logger::initialize("logs/pkt_sending_queue_add_unittest.log", spdlog::level::debug);
    auto queue = create_queue(0, 500, 0xFFFFFFFF, 8192, false);
    auto str = std::make_shared<std::string>("0");
    for (int i = 0; i < 8192; i++) {
        queue->input_packet(str, 0, 0);
    }
    EXPECT_EQ(queue->capacity(), 0);
    EXPECT_EQ(queue->get_buffer_size(), 8192);
    queue->input_packet(str, 0, 0);
    EXPECT_EQ(queue->get_buffer_size(), 8192);
    /// 1 - 8192
    EXPECT_EQ(queue->get_first_block()->seq, 1);
    queue->ack_sequence_to(10);
    EXPECT_EQ(queue->get_first_block()->seq, 10);
    EXPECT_EQ(queue->get_buffer_size(), 8183);
    /// 10 - 8192
    queue->ack_sequence_to(50);
    EXPECT_EQ(queue->get_buffer_size(), 8143);
    queue->ack_sequence_to(10000);
    EXPECT_EQ(queue->get_buffer_size(), 0);
}


TEST(timed_ack_sequence_to, packet_sending_queue) {

    logger::initialize("logs/pkt_sending_queue_ack_to_unittest.log", spdlog::level::debug);
    auto str = std::make_shared<std::string>("0");
    auto queue = create_queue(16, 0xFFFFFFFF, 20, 8, true);

    auto poller = queue->get_poller();
    std::atomic<bool> f{false};
    poller->async([&]() {
        queue->input_packet(str, 0, 0);//16
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//17
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//18
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//19
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//0
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//1
        SLEEP_MS(400);
        queue->input_packet(str, 0, 0);//2
        SLEEP_MS(400);
        f.store(true);
    });
    bool _ = true;
    while (!f.compare_exchange_weak(_, true)) _ = true;
    f.store(false);
    SLEEP_SECOND(3);
    poller->async([&]() {
        queue->ack_sequence_to(15);
        EXPECT_EQ(queue->get_buffer_size(), 7);
        queue->ack_sequence_to(2);
        EXPECT_EQ(queue->get_buffer_size(), 1);
        f.store(true);
    });

    _ = true;
    while (!f.compare_exchange_weak(_, true)) _ = true;

    EXPECT_EQ(1, 1);


}