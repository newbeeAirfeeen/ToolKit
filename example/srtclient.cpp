//
// Created by 沈昊 on 2022/8/7.
//
#include "Util/semaphore.h"
#include "protocol/srt/srt_error.hpp"
#include <csignal>
#include <iostream>
#include <protocol/srt/srt_client.hpp>
#include <spdlog/logger.hpp>
using namespace std;

std::unique_ptr<std::thread> worker;
std::atomic<bool> _quit{false};
void on_connected(const std::shared_ptr<srt::srt_client> &client) {
    int counts = 1;
    std::string str = "client to send ";
    while (!_quit.load()) {
        std::string send_buf = str + std::to_string(counts++);
        client->async_send(send_buf.data(), send_buf.size());
        std::this_thread::sleep_for(std::chrono::milliseconds (1));
    }
}

int main(int argc, char **argv) {

    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });
    logger::initialize("logs/srt_client.log", spdlog::level::info);

    if (argc < 3) {
        std::cerr << "srt_client <remote_ip> <remote port>";
        return -1;
    }


    asio::ip::udp::endpoint remote(asio::ip::make_address(argv[1]), std::stoi(argv[2]));
    auto client = std::make_shared<srt::srt_client>();

    client->set_on_receive([&](const std::shared_ptr<buffer> &buff) {
        Info("receive: {}", buff->data());
    });

    client->set_on_error([&, client](const std::error_code &e) {
        Error("err: {}", e.message());
        _quit.store(true);
    });

    client->async_connect(remote, [&](const std::error_code &e) {
        if (e) {
            Error("connect result: {}", e.message());
            return;
        }
        worker.reset(new std::thread(on_connected, client));
    });

    Info("wait to quit");
    sem.wait();
    return 0;
}