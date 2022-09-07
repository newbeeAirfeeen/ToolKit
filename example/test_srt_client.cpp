//
// Created by 沈昊 on 2022/8/7.
//
#include "protocol/srt/srt_error.hpp"
#include <protocol/srt/srt_client.hpp>
#include <spdlog/logger.hpp>
#include "net/event_poller_pool.hpp"
using namespace std;
void send_data(const std::shared_ptr<srt::srt_client> &client) {
//    Info("begin send data..");
//    for (int i = 0; i < 1000000; i++) {
//        auto ret = client->async_send("this is client message!", 23);
//        if (ret == 0) {
//            i--;
//        }
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }

    client->async_send("this is client message!", 23);
}

int main() {
    logger::initialize("logs/test_srt_client.log", spdlog::level::info);

    auto poller = event_poller_pool::Instance().get_poller(false);

    asio::ip::udp::endpoint p(asio::ip::udp::v4(), 9000);
    //asio::ip::udp::endpoint p(asio::ip::make_address("49.235.73.47"), 9000);
    asio::ip::udp::endpoint p2(asio::ip::udp::v4(), 12012);
    auto client = std::make_shared<srt::srt_client>(poller, p2);

    client->set_on_receive([&](const std::shared_ptr<buffer> &buff) {
        Info("receive: {}", buff->data());
        client->async_send("client data", 11);
    });

    client->set_on_error([&, client](const std::error_code &e) {
        Error(e.message());
    });


    client->async_connect(p, [&](const std::error_code &e) {
        if (e) {
            Error(e.message());
            return;
        }
        Warn("connect success");
        return send_data(client);
    });


    event_poller_pool::Instance().wait();
    return 0;
}