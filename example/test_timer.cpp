//
// Created by 沈昊 on 2022/4/9.
//
#include <event_poller.hpp>
#include <iostream>
#include <net/asio.hpp>
#include <thread>
using namespace std;
int main(){



    asio::io_context t;
    asio::io_context t2;

    asio::ip::udp::socket sock(t);



    std::thread ths([&](){
        t.run();
    });


    std::thread thss([&](){
        t.run();
    });


    t2.run();









    ths.join();
    thss.join();
    return 0;
}