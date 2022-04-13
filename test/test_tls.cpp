//
// Created by 沈昊 on 2022/4/12.
//
#include <net/tcp_server.hpp>
#include <net/ssl/engine.hpp>
#include <net/ssl/context.hpp>
#include <net/tcp_session.hpp>
#include <net/event_poller.hpp>
#include <net/event_poller_pool.hpp>
#include <net/ssl/tls.hpp>
int main(){

    logger::initialize("logs/test_tcp_server.log", spdlog::level::trace);
    auto& pool = event_poller_pool::Instance();
    auto http_server = std::make_shared<tcp_server>();
#ifdef SSL_ENABLE
    std::shared_ptr<context> _context = std::make_shared<context>(context::tls::method::sslv23_server);
    _context->use_certificate_chain_file("default.pem");
    _context->use_private_key_file("default.pem", context::pem);
    _context->set_verify_mode(context::verify_none);
    http_server->start<tls<tcp_session>>(443, "0.0.0.0", true, _context);
    //http_server->start<tls<tcp_client>>(8080);
#endif

    pool.wait();
    
    return 0;
}



