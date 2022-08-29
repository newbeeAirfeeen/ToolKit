//
// Created by 沈昊 on 2022/4/9.
//
#include <iostream>
#include <net/asio.hpp>
#include <protocol/srt/deadline_timer.hpp>
#include <spdlog/logger.hpp>
#include <thread>
using namespace std;
int main() {

    logger::initialize("logs/test_timer.log", spdlog::level::trace);
    asio::io_context context(1);
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());

    auto timer = create_deadline_timer<std::string, std::chrono::seconds>(context);


    static int couple = 0;
    timer->set_on_expired([timer](const std::string& str) {
        auto c = timer->get_last_time_expired() + 1;
        Info("last time expired={}", timer->get_last_time_expired());
        timer->expired_at(c, "1");
        Info("last time expired={}", timer->get_last_time_expired());
        timer->expired_at(timer->get_last_time_expired() + 1, "2");
        Info("last time expired={}", timer->get_last_time_expired());
        timer->expired_at(timer->get_last_time_expired() + 1, "3");
        Info("last time expired={}", timer->get_last_time_expired());
    });

    auto begin = std::chrono::steady_clock::now();
    timer->add_expired_from_now(0, std::to_string(0));
    context.run();

    auto end = std::chrono::steady_clock::now();
    auto spend = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() / 1000 * 1.0;
    Warn("time past {} s", spend);

    return 0;
}