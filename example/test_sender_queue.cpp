//
// Created by 沈昊 on 2022/8/12.
//
#include <protocol/srt/sliding_window.hpp>
#include <spdlog/logger.hpp>

class basic_sender : public sliding_window<int>, public std::enable_shared_from_this<basic_sender> {
public:
    explicit basic_sender(asio::io_context &context) : sliding_window<int>(context) {
    }

protected:
    pointer get_shared_from_this() override {
        return shared_from_this();
    }


public:
    void on_packet(const block_type &type) override {
        Info("on packet : {}, value={}", type->sequence_number, type->content);
    }

    void on_drop_packet(size_type begin, size_type end) override {
        Info("drop packet {}-{}", begin, end);
    }

public:
    void send_to(int v) {
        auto b = std::make_shared<block>();
        b->content = v;
    }

private:
};
int main() {

    logger::initialize("logs/test_sender_queue.log", spdlog::level::trace);

    asio::io_context io(1);
    asio::executor_work_guard<typename asio::io_context::executor_type> guard(io.get_executor());
    auto sender = std::make_shared<basic_sender>(io);
    sender->set_max_delay(0);
    sender->set_initial_sequence(2);
    sender->set_max_sequence(10);
    sender->set_window_size(8);
    sender->start();
    static int vv = 1;
    auto t = create_deadline_timer<int>(io);
    t->set_on_expired([&](const int &v) {
        io.post([&]() {
            std::static_pointer_cast<basic_sender>(sender)->send_to(vv++);
        });
        t->add_expired_from_now(200, 1);
    });
    t->add_expired_from_now(0, 1);
    io.run();
}