//
// Created by 沈昊 on 2022/4/13.
//
#include <net/udp_client.hpp>
#include <event_poller_pool.hpp>
#include <spdlog/logger.hpp>
#include <net/ssl/dtls.hpp>
int main(){

    logger::initialize("logs/test_udp_client.log", spdlog::level::trace);
    auto client = std::make_shared<udp_client>(*event_poller_pool::Instance().get_poller(false));
    client->bind_local(9111);

    client->setOnError([](const std::error_code& e, const typename udp_client::endpoint_type& endpoint){
              Error(e.message());
          });

    client->connect(asio::ip::udp::socket::endpoint_type(asio::ip::make_address("0.0.0.0"), 9000));
    basic_buffer<char> buf("shenhao", 7);
    client->send(buf);
    client->connect(asio::ip::udp::socket::endpoint_type(asio::ip::make_address("0.0.0.0"), 9001));

    buf.append("one two");
    client->send(buf);


    event_poller_pool::Instance().wait();



    return 0;
}