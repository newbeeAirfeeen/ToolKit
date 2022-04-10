//
// Created by 沈昊 on 2022/4/10.
//
#include "socket_base.hpp"
#include "event_poller_pool.hpp"
std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> socket_helper::create_bind_socket(){
    auto poller = event_poller_pool::Instance().get_poller(false);
    return std::make_pair(poller, std::make_shared<asio::ip::tcp::socket>(poller->get_executor()));
}
