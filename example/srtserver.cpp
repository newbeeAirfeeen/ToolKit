//
// Created by 沈昊 on 2022/8/20.
//
#include "Util/semaphore.h"
#include "event_poller_pool.hpp"
#include "protocol/srt/srt_server.hpp"
#include <iostream>
#include <spdlog/logger.hpp>
#include <string>
#ifdef ENABLE_PREF_TOOL
#include <gperftools//profiler.h>
#endif
using namespace srt;

class only_receive_session : public srt_session {
public:
    only_receive_session(const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context) : srt_session(sock, context) {
    }
    ~only_receive_session() override = default;

protected:
    void onRecv(const std::shared_ptr<buffer> &ptr) override {
        Info("receive: {}", ptr->data());
    }
    void onError(const std::error_code &e) override {
        srt_session::onError(e);
    }
};

class echo_session : public only_receive_session {
public:
    echo_session(const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context) : only_receive_session(sock, context) {
    }
protected:
    void onRecv(const std::shared_ptr<buffer> &ptr) override {
        Info("receive: {}", ptr->data());
        auto str = std::to_string(counts++);
        async_send(str.data(), str.size());
    }
private:
    uint32_t counts = 1;
};


int main(int argc, char **argv) {
    const char *exec_name = "srtserver.profile";
#ifdef ENABLE_PREF_TOOL
    ProfilerStart(exec_name);
#endif
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });

    logger::initialize("logs/srt_server.log", spdlog::level::trace);

    if (argc < 2) {
        std::cerr << "srtserver <local port>";
        return -1;
    }

    auto server = std::make_shared<srt_server>();
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), std::stoi(argv[1]));

    /// 设置创建session回调
    server->on_create_session([](const std::shared_ptr<asio::ip::udp::socket> &sock, const event_poller::Ptr &context) -> std::shared_ptr<srt_session> {
        return std::make_shared<echo_session>(sock, context);
    });

    server->start(endpoint);

    sem.wait();
#ifdef ENABLE_PREF_TOOL
    ProfilerStop();
#endif
    return 0;
}