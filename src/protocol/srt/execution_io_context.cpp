//
// Created by 沈昊 on 2022/8/23.
//
#include "execution_io_context.hpp"
io_context::io_context() {
    std::atomic<bool> is_running{false};
    _thread.reset(new std::thread([&]() {
        _id = std::this_thread::get_id();
        is_running.store(true);
        this->run();
    }));
    bool _ = true;
    is_running.compare_exchange_strong(_, true);
}

io_context::~io_context() {
    _context.post([]() { throw thread_exit_exception(); });
    if (_thread && _thread->joinable()) _thread->join();
}

const std::thread::id &io_context::get_thread_id() const {
    return this->_id;
}

void io_context::run() {
    while (true) {
        try {
            asio::executor_work_guard<typename asio::io_context::executor_type> guard(_context.get_executor());
            _context.run();
        } catch (const thread_exit_exception &) { break; } catch (const std::exception&) {
        }
    }
}

io_pool::io_pool() {
    _pool_.reset(new std::list<std::shared_ptr<io_context>>());
    auto size = std::thread::hardware_concurrency() / 4;
    if(size <= 0) size = 1;
    for (decltype(size) i = 0; i < size; i++) {
        _pool_->emplace_back(std::make_shared<io_context>());
    }
}

io_pool::~io_pool() {}

io_pool &io_pool::instance() {
    static io_pool pool;
    return pool;
}

void io_pool::for_each(const std::function<void(io_context &)> &f) {
    std::for_each(_pool_->begin(), _pool_->end(), [f](const std::shared_ptr<io_context> &io) {
        if (io) f(*io);
    });
}

asio::thread_pool &get_thread_pool() {
    static uint32_t size = std::thread::hardware_concurrency() / 4;
    if (size <= 0) size = 1;
    static asio::thread_pool pool(size);
    return pool;
}