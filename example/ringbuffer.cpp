//
// Created by 沈昊 on 2022/9/15.
//
#include "Util/RingBuffer.h"
#include "Util/semaphore.h"
#include "net/event_poller.hpp"
#include "net/event_poller_pool.hpp"
#include "spdlog/logger.hpp"
#include <csignal>
#include "Util/string_view.h"
#include "Util/optional.hpp"
int main() {
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });
    logger::initialize("logs/ringbuffer.log", spdlog::level::info);


    auto ring_buffer = std::make_shared<toolkit::RingBuffer<int>>();


    auto poller = event_poller_pool::Instance().get_poller(false);
    std::shared_ptr<toolkit::RingBuffer<int>::RingReader> reader;
    poller->async([&]() {
        reader = ring_buffer->attach(poller);
        reader->setReadCB([](int v) {
            Info("read cb: {}", v);
        });

        reader->setDetachCB([]() {
            Warn("detach ring reader..");
        });
        Info("attach ring buffer...");
    });

    static int v = 1;
    auto poller_2 = event_poller_pool::Instance().get_poller(false);
    auto timer = poller_2->do_delay_task(std::chrono::milliseconds(1000), [&]() -> size_t {
        Warn("write value to other poller");
        ring_buffer->write(v++);
        return 1000;
    });




    sem.wait();
    return 0;
}