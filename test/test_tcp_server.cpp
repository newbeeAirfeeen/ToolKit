//
// Created by 沈昊 on 2022/4/4.
//
#include <net/tcp_server.hpp>
#include <iostream>
#include <spdlog/logger.hpp>
#include <net/event_poller_pool.hpp>
using namespace std;
int main(){
    logger::initialize("logs/test_tcp_server.log", spdlog::level::trace);
    auto& pool = event_poller_pool::Instance();
    auto http_server = std::make_shared<tcp_server>();
    //http_server->start<tcp_session>(9000);
//    std::shared_ptr<asio::ssl::context> context = std::make_shared<asio::ssl::context>(asio::ssl::context::method::sslv23_server);
//    context->use_certificate_chain_file("default.pem");
//    context->use_private_key_file("default.pem", asio::ssl::context::pem);
//    context->set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
//    http_server->start<tls_session>(443, "0.0.0.0", true, context);
    pool.wait();
    return 0;
}