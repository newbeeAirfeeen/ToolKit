//
// Created by 沈昊 on 2022/8/23.
//

#ifndef TOOLKIT_IO_CONTEXT_HPP
#define TOOLKIT_IO_CONTEXT_HPP
#include "net/asio.hpp"
#include <atomic>
#include <list>
#include <memory>
#include <thread>
struct thread_exit_exception : public std::exception {};
class io_context {
public:
    io_context();
    ~io_context();
    asio::io_context &get_poller() {
        return std::ref(_context);
    }

    const std::thread::id &get_thread_id() const;

private:
    void run();

private:
    std::unique_ptr<std::thread> _thread;
    asio::io_context _context{1};
    std::thread::id _id;
};

class io_pool {
public:
    ~io_pool();
    static io_pool &instance();
    void for_each(const std::function<void(io_context &)> &f);

private:
    io_pool();

private:
    std::unique_ptr<std::list<io_context *>> _pool_;
};

#endif//TOOLKIT_IO_CONTEXT_HPP
