//
// Created by 沈昊 on 2022/4/12.
//
#include "tcp_client.hpp"
#include "Util/onceToken.h"
#include "socket_base.hpp"
using super_type = typename tcp_client::super_type;

#ifdef SSL_ENABLE
tcp_client::tcp_client(bool current_thread, const std::shared_ptr<context> &_context_) : tcp_session(socket_helper::create_bind_socket(current_thread), _context_) {}
#else
tcp_client::tcp_client(bool current_thread) : tcp_session(socket_helper::create_bind_socket(current_thread)) {}
#endif

void tcp_client::on_connected() {
    Info("connected");
}

void tcp_client::onRecv(const char *data, size_t size) {
    Info("client recv {} length", size);
}

void tcp_client::bind_local(const std::string &ip, unsigned short port) {
    using endpoint = typename asio::ip::tcp::endpoint;
    auto address = asio::ip::make_address(ip);
    endpoint end_point(address, port);
    this->local_point = end_point;
}

void tcp_client::start_connect(const std::string &ip, unsigned short port) {
    toolkit::onceToken token([&]() { mtx.lock(); }, [&]() { mtx.unlock(); });
    close();
    super_type::bind(this->local_point);
    endpoint end_point(asio::ip::make_address(ip), port);
    auto stronger_self(std::static_pointer_cast<tcp_client>(shared_from_this()));
    super_type::async_connect(end_point, [stronger_self](const std::error_code &e) {
        if (e) {
            Error(e.message());
            stronger_self->onError(e);
            return;
        }
        stronger_self->on_connected();
        return stronger_self->read_l();
    });
}