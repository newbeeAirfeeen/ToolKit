//
// Created by 沈昊 on 2022/8/20.
//
#include "protocol/srt/srt_server.hpp"
#include <spdlog/logger.hpp>
using namespace srt;

class srt_echo_session : public srt_session {
public:
    srt_echo_session(const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &context) : srt_session(sock, context) {}
    ~srt_echo_session() override = default;
protected:
    void onRecv(const std::shared_ptr<buffer> &ptr) override {
        Info("recv: {}", ptr->data());
        async_send("server msg", 10);
    }
    void onError(const std::error_code &e) override {
        srt_session::onError(e);
    }
};


int main() {


    logger::initialize("logs/test_srt_server.log", spdlog::level::info);

    asio::io_context context;
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());
    auto server = std::make_shared<srt_server>();
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), 9000);

    /// 设置创建session回调
    server->on_create_session([](const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &context) -> std::shared_ptr<srt_session> {
        return std::make_shared<srt_echo_session>(sock, context);
    });

    server->start(endpoint);
    context.run();
    return 0;
}