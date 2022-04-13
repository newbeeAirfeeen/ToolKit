//
// Created by 沈昊 on 2022/4/9.
//
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <net/udp_server.hpp>

int main(){
    asio::io_service service;
    asio::ip::udp::socket udp_socket(service);
//  asio::ssl::dtls ctx(asio::ssl::context::no_sslv3);
//  asio::ssl::stream<asio::ip::udp::socket> strem_(service, ctx);



    return 0;
}