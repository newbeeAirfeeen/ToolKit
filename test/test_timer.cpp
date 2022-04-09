//
// Created by 沈昊 on 2022/4/9.
//
#include <event_poller.hpp>
#include <iostream>
using namespace std;
int main(){
    logger::initialize("logs/test_timer.log", spdlog::level::info);
    auto ev_poller = std::make_shared<event_poller>();
    ev_poller->start();
    ev_poller->execute_delay_task(std::chrono::milliseconds(1000), []() -> size_t{

        cout << "Hello,world" << endl;
        return 1000;
    });

    ev_poller->join();
    return 0;
}