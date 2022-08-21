//
// Created by 沈昊 on 2022/8/20.
//
#include "protocol/srt/srt_server.hpp"
#include <spdlog/logger.hpp>
using namespace srt;


int main(){


    logger::initialize("logs/test_logger.log", spdlog::level::trace);

    Trace("{}", __cplusplus);


    asio::io_context context;
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(context.get_executor());
    auto server = std::make_shared<srt_server>();
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), 9000);
    server->start(endpoint);
    context.run();
    return 0;
}