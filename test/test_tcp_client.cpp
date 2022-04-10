//
// Created by 沈昊 on 2022/4/10.
//
#include <event_poller_pool.hpp>
#include "net/tcp_client.hpp"
int main(){

    logger::initialize("logs/test_client.log", spdlog::level::trace);
    //asio::io_context context;
    //asio::ssl::stream<asio::ip::tcp::socket> sock(context);
    auto& pool = event_poller_pool::Instance();
    std::shared_ptr<asio::ssl::context> context = std::make_shared<asio::ssl::context>(asio::ssl::context::method::sslv23_client);
    context->use_certificate_chain_file("default.pem");
    context->use_private_key_file("default.pem", asio::ssl::context::pem);
    context->set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
    auto client = std::make_shared<tls_client>(*pool.get_poller(false), context);

    client->start_connect("127.0.0.1", 8080);



    pool.wait();
    return 0;
}
