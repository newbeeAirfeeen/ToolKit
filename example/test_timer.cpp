//
// Created by 沈昊 on 2022/4/9.
//
#include <iostream>
#include <net/asio.hpp>
#include <protocol/srt/deadline_timer_queue.hpp>
#include <thread>
#include <spdlog/logger.hpp>
using namespace std;
int main() {

    logger::initialize("logs/test_timer.log", spdlog::level::trace);
    asio::io_context context;

    static std::atomic<int> counts{0};
    auto timer = create_deadline_timer<std::string>(context);


    timer->set_on_expired([timer](const std::string& str) {
        Info("tag: {}", str);
    });

    timer->add_expired_from_now(1000, "1000");
    timer->add_expired_from_now(1225, "1225");
    timer->add_expired_from_now(1445, "1445");
    timer->add_expired_from_now(1678, "1678");
    timer->add_expired_from_now(2111, "2111");
    timer->add_expired_from_now(210, "210");
    timer->add_expired_from_now(111, "111");
    timer->add_expired_from_now(120, "120");
    timer->add_expired_from_now(240, "240");
    timer->add_expired_from_now(11240, "11240");


    context.run();
    return 0;
}