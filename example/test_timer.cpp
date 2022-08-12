//
// Created by 沈昊 on 2022/4/9.
//
#include <iostream>
#include <net/asio.hpp>
#include <protocol/srt/deadline_timer_queue.hpp>
#include <spdlog/logger.hpp>
#include <thread>
using namespace std;
int main() {

    logger::initialize("logs/test_timer.log", spdlog::level::trace);
    asio::io_context context(1);
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());

    auto timer = create_deadline_timer<std::string, std::chrono::microseconds>(context);


    static int couple = 0;
    timer->set_on_expired([timer](const std::string& str) {
        Info("tag {}:{}", couple++, str);
        timer->add_expired_from_now(1000, std::to_string(couple) + " 2000 end");
    });

    auto begin = std::chrono::steady_clock::now();
    timer->add_expired_from_now(0, std::to_string(0));
//    std::thread t([&](){
//        for(int i = 0 ;i < 10;i++){
//            timer->add_expired_from_now(1000, std::to_string(1000));
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        }
//    });
    context.run();
    //t.join();

    auto end = std::chrono::steady_clock::now();

    auto spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000 * 1.0;

    Warn("time past {} s", spend);

    return 0;
}