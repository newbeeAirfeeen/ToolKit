//
// Created by 沈昊 on 2022/4/10.
//
#include "net/deprecated/event_poller_pool.hpp"
#include "net/deprecated/tcp_client.hpp"
#include "net/deprecated/tcp_server.hpp"
#include "net/deprecated/tcp_session.hpp"
#include <iostream>

int main() {

    logger::initialize("logs/test_client.log", spdlog::level::trace);
    auto &pool = event_poller_pool::Instance();

    auto server = std::make_shared<tcp_server>();
#ifdef OPENSSL_ENABLE
    std::shared_ptr<asio::ssl::context> context = std::make_shared<asio::ssl::context>(asio::ssl::context::method::sslv23_client);
    auto _tls = std::make_shared<tls<tcp_client>>(*event_poller_pool::Instance().get_poller(false), context);

    std::shared_ptr<asio::ssl::context> server_context = std::make_shared<asio::ssl::context>(asio::ssl::context::method::sslv23_server);
    server_context->use_certificate_chain_file("default.pem");
    server_context->use_private_key_file("default.pem", asio::ssl::context::pem);
    server_context->set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
    server->start<tls<tcp_session>>({asio::ip::address_v4::loopback(), 4430}, server_context);

    buffer buf("shenhao");
    //_tls->async_send(buf);
    //_tls->async_connect({asio::ip::address_v4::loopback(), 9000});
#endif
    server->start<tcp_session>({asio::ip::address_v4::loopback(), 12000});
    auto tcp = std::make_shared<tcp_client>(*event_poller_pool::Instance().get_poller(false));


    std::string buf_;
    while (getline(std::cin, buf_)) {
        buffer buff(std::move(buf_));
        tcp->async_send(buff);
    }

    pool.wait();
    return 0;
}
