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
#include <gperftools/profiler.h>
#endif
using namespace srt;


int main(int argc, char **argv) {
    const char *exec_name = "srtserver.profile";
#ifdef ENABLE_PREF_TOOL
    ProfilerStart(exec_name);
#endif
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });

    logger::initialize("logs/srt_server.log", spdlog::level::info);

    if (argc < 2) {
        std::cerr << "srtserver <local port>";
        return -1;
    }

    auto server = std::make_shared<srt_server>();
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), std::stoi(argv[1]));
    server->start(endpoint);

    sem.wait();
#ifdef ENABLE_PREF_TOOL
    ProfilerStop();
#endif
    return 0;
}