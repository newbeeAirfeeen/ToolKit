//
// Created by 沈昊 on 2022/4/13.
//
#include <net/udp_client.hpp>
#include <event_poller_pool.hpp>
#include <spdlog/logger.hpp>
int main(){

    logger::initialize("logs/test_udp_client.log", spdlog::level::trace);
    auto client = std::make_shared<udp_client>(*event_poller_pool::Instance().get_poller(false));
    client->bind_local(9111);
    client->connect(asio::ip::udp::socket::endpoint_type(asio::ip::make_address("0.0.0.0"), 9000));
    basic_buffer<char> buf("shenhao", 7);
    client->send(buf);
    basic_buffer<char> vuff;
    vuff.append("sdflskdf",8);
    buf.append("hello,world");
    client->send(buf);
    //client->connect(asio::ip::udp::socket::endpoint_type(asio::ip::make_address("0.0.0.0"), 9000));
    client->send(vuff);
    event_poller_pool::Instance().wait();



    return 0;
}