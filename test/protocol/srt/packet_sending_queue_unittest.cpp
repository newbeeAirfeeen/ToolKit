//
// Created by 沈昊 on 2022/8/5.
//
#include <gtest/gtest.h>
#include <iostream>
#include "net/event_poller_pool.hpp"
#include "protocol/srt/packet_sending_queue.hpp"
#include "protocol/srt/srt_ack.hpp"
using namespace std;
using namespace srt;
TEST(queue, packet_sending_queue){
    auto poller = event_poller_pool::Instance().get_poller(false);
    auto ack_queue_ = std::make_shared<srt_ack_queue>();
    auto queue = std::make_shared<packet_sending_queue<std::shared_ptr<std::string>>>(poller, ack_queue_);






}