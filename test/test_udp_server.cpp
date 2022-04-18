//
// Created by 沈昊 on 2022/4/9.
//
#ifdef SSL_ENABLE
#include <asio/ssl.hpp>
#endif
#include <asio.hpp>
#include <event_poller_pool.hpp>
#include <net/udp_server.hpp>
#include <spdlog/logger.hpp>
int main() {
    logger::initialize("logs/test_tcp_server.log", spdlog::level::trace);
    auto udp_server_ = std::make_shared<udp_server>(*event_poller_pool::Instance().get_poller(false));
    udp_server_->start<udp_session>(9000);
    auto udp_server_2 = std::make_shared<udp_server>(*event_poller_pool::Instance().get_poller(false));
    udp_server_2->start<udp_session> (9001);





    event_poller_pool::Instance().wait();
    return 0;
}