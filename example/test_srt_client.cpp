//
// Created by 沈昊 on 2022/8/7.
//
#include <protocol/srt/srt_client.hpp>
#include <spdlog/logger.hpp>
using namespace std;
int main() {
    logger::initialize("logs/test_logger.log", spdlog::level::trace);

    asio::io_context context(1);

    asio::ip::udp::endpoint p(asio::ip::make_address("127.0.0.1"), 9000);
    asio::ip::udp::endpoint p2(asio::ip::make_address("127.0.0.1"), 10000);
    auto client = std::make_shared<srt::srt_client>(context, p2);
    client->async_connect(p, [](const std::error_code &e) {

    });

    context.run();
    return 0;
}