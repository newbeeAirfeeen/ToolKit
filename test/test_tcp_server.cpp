//
// Created by 沈昊 on 2022/4/4.
//
#include <net/tcp_server.hpp>
#include <iostream>
#include <spdlog/logger.hpp>
#include <net/event_poller_pool.hpp>
#include <buffer_list.hpp>
#include "net/ssl/context.hpp"
#include "net/ssl/ssl.hpp"
using namespace std;
int main(){
    logger::initialize("logs/test_tcp_server.log", spdlog::level::trace);
    auto& pool = event_poller_pool::Instance();
    auto http_server = std::make_shared<tcp_server>();
    http_server->start<tcp_session>(8080);
#ifdef SSL_ENABLE
    std::shared_ptr<context> _context = std::make_shared<context>(context::tls::method::sslv23_server);
    _context->use_certificate_chain_file("default.pem");
    _context->use_private_key_file("default.pem", context::pem);
    _context->set_verify_mode(context::verify_fail_if_no_peer_cert);
    http_server->start<ssl<tcp_session>>(443, "0.0.0.0", true, _context);
#endif
    pool.wait();

    return 0;
}