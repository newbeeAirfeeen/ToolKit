//
// Created by 沈昊 on 2022/4/4.
//
#include <net/event_poller.hpp>
#include <net/event_poller_pool.hpp>
#include <spdlog/logger.hpp>
#include <iostream>
using namespace std;
void test_event_poller(){

    event_poller::Ptr poller = std::make_shared<event_poller>();
    poller->start();

    poller->async([](){
        cout << "Hello,world" << endl;
    });
    cin.get();
}
void test_event_pool(){
    event_poller_pool::Instance();
    cin.get();
}
int main(){

    logger::initialize("test_event_poller_pool", spdlog::level::debug);
    //test_event_poller();
    test_event_pool();
    return 0;
}



