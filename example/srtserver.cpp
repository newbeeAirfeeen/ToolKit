//
// Created by 沈昊 on 2022/8/20.
//
#include "Util/semaphore.h"
#include "event_poller_pool.hpp"
#include "protocol/srt/srt_server.hpp"
#include <iostream>
#include <spdlog/logger.hpp>
#include <string>
using namespace srt;

class srt_echo_session : public srt_session {
public:
    srt_echo_session(const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context) : srt_session(sock, context) {
        str = "this is server msg ";
    }
    ~srt_echo_session() override = default;

protected:
    void onRecv(const std::shared_ptr<buffer> &ptr) override {
        Info("receive: {}", ptr->data());
        std::string strr = str + std::to_string(count++);
        async_send(strr.data(), strr.size());
    }
    void onError(const std::error_code &e) override {
        srt_session::onError(e);
    }

private:
    int count = 1;
    std::string str;
};


int main(int argc, char **argv) {
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });

    logger::initialize("logs/srt_server.log", spdlog::level::info);

    if (argc < 2) {
        std::cerr << "srtserver <local port>";
        return -1;
    }

    auto server = std::make_shared<srt_server>();
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), std::stoi(argv[1]));

    /// 设置创建session回调
    server->on_create_session([](const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context) -> std::shared_ptr<srt_session> {
        return std::make_shared<srt_echo_session>(sock, context);
    });

    server->start(endpoint);

    sem.wait();
    return 0;
}