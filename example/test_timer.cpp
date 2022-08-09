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
    asio::io_context context;

    auto timer = create_deadline_timer<std::string>(context);


    static int couple = 1;
    timer->set_on_expired([timer](const std::string& str) {
        ///Info("tag {}:{}", couple++, str);
    });

    auto begin = std::chrono::steady_clock::now();
    std::thread t([&](){
        auto begin = std::chrono::steady_clock::now();
        for(int i = 0 ;i < 10;i++){
            Info("insert");
            timer->add_expired_from_now(1000, std::to_string(1000));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    context.post([](){

    });
    context.run();
    t.join();

    auto end = std::chrono::steady_clock::now();

    auto spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000 * 1.0;

    Warn("time past {} s", spend);

    return 0;
}