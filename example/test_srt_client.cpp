//
// Created by 沈昊 on 2022/8/7.
//
#include <protocol/srt/srt_client.hpp>
#include <spdlog/logger.hpp>
#include "protocol/srt/srt_server.hpp"
using namespace std;

void send_data(const std::shared_ptr<srt::srt_client> &client) {

    Info("begin send data..");
    for (int i = 0; i < 10000; i++) {
        client->async_send(std::to_string(i + 1).data(), 1);
        std::this_thread::sleep_for(std::chrono::milliseconds (1));
    }
}
#include "protocol/srt/srt_error.hpp"
int main() {
    logger::initialize("logs/test_logger.log", spdlog::level::trace);

    Info("{}", sizeof(srt::srt_socket_service));

    asio::io_context context(1);
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());
    asio::ip::udp::endpoint p(asio::ip::make_address("127.0.0.1"), 9000);
    asio::ip::udp::endpoint p2(asio::ip::make_address("127.0.0.1"), 21000);
    auto client = std::make_shared<srt::srt_client>(context, p2);

    std::thread t([&]() { context.run(); });
    std::shared_ptr<std::thread> t2;

    client->set_on_error([&, client](const std::error_code &e) {
        Info(e.message());
        if (e.value() == srt::srt_error_code::success) {
            t2.reset(new std::thread(send_data, client));
        }
    });
    client->async_connect(p, [&, client](const std::error_code& e){
        if(e){
            return;
        }
        Warn("connect success");
        t2.reset(new std::thread(send_data, client));
    });

    t.join();
    if (t2 && t2->joinable()) {
        t2->join();
    }
    return 0;
}