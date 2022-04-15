//
// Created by 沈昊 on 2022/4/9.
//
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <event_poller_pool.hpp>
#include <net/udp_server.hpp>
#include <spdlog/logger.hpp>
int main() {
    logger::initialize("logs/test_tcp_server.log", spdlog::level::trace);
    auto udp_server_ = std::make_shared<udp_server>(*event_poller_pool::Instance().get_poller(false));
    udp_server_->start<udp_session>(9000);
    event_poller_pool::Instance().wait();
    return 0;
}