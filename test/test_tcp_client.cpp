//
// Created by 沈昊 on 2022/4/10.
//
#include <event_poller_pool.hpp>
#include "net/tcp_client.hpp"
#include <vector>
int main(){

    logger::initialize("logs/test_client.log", spdlog::level::trace);
    //asio::io_context context;
    //asio::ssl::stream<asio::ip::tcp::socket> sock(context);
    auto& pool = event_poller_pool::Instance();
    std::shared_ptr<asio::ssl::context> context = std::make_shared<asio::ssl::context>(asio::ssl::context::method::sslv23_client);
    context->use_certificate_chain_file("default.pem");
    context->use_private_key_file("default.pem", asio::ssl::context::pem);
    context->set_verify_mode(asio::ssl::verify_fail_if_no_peer_cert);
    std::vector<std::shared_ptr<tcp_client>> vec;
    for(int i = 0; i < 100000;i++){
        auto client = std::make_shared<tcp_client>();
        client->start_connect("49.235.73.47", 9000);
        vec.push_back(client);
    }



    pool.wait();
    return 0;
}
