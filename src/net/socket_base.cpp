//
// Created by 沈昊 on 2022/4/10.
//
#include "socket_base.hpp"
#include "event_poller_pool.hpp"
std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> socket_helper::create_tcp_socket(bool current_thread){
    auto poller = event_poller_pool::Instance().get_poller(current_thread);
    return std::make_pair(poller, std::make_shared<asio::ip::tcp::socket>(poller->get_executor()));
}

std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::udp::socket>> socket_helper::create_udp_socket(bool current_thread){
    auto poller = event_poller_pool::Instance().get_poller(current_thread);
    return std::make_pair(poller, std::make_shared<asio::ip::udp::socket>(poller->get_executor()));
}