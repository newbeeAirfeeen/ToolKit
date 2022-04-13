//
// Created by 沈昊 on 2022/4/12.
//
#include "tcp_session.hpp"
#include "session_helper.hpp"
#include "spdlog/logger.hpp"
#include <chrono>

#ifdef SSL_ENABLE
tcp_session::tcp_session(typename asio::ip::tcp::socket &socket_, event_poller &poller,
                         const std::shared_ptr<context> &context) : poller(poller), asio::ip::tcp::socket(std::move(socket_)),
                                                                    _context(context), recv_timer(poller.get_executor()),
                                                                    send_timer(poller.get_executor()) {
}
tcp_session::tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> &pair_,
                         const std::shared_ptr<context> &_context_)
    : _context(_context_), recv_timer(pair_.first->get_executor()), send_timer(pair_.first->get_executor()), asio::ip::tcp::socket(std::move(*pair_.second)), poller(*pair_.first) {
    _is_server = false;
}
#else
tcp_session::tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>> &pair_)
    : recv_timer(pair_.first->get_executor()), send_timer(pair_.first->get_executor()), poller(*pair_.first), super_type(std::move(*pair_.second)) {
}
tcp_session::tcp_session(super_type &socket_, event_poller &poller)
    : poller(poller), asio::ip::tcp::socket(std::move(socket_)), recv_timer(poller.get_executor()),
      send_timer(poller.get_executor()) {}
#endif
tcp_session::~tcp_session() {
    Trace("~session");
    super_type::close();
}

event_poller &tcp_session::get_poller() {
    return this->poller;
}

void tcp_session::onRecv(const char *data, size_t length) {
    Info("recv data {} length", length);
    send("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nyes", 67);
}

void tcp_session::onError(const std::error_code &e) {
    Error(e.message());
}

void tcp_session::before_begin_session() {
}

void tcp_session::set_send_time_out(size_t time) {
    send_time_out.store(time);
    this->send_timer.expires_after(std::chrono::seconds(time));
}

void tcp_session::set_recv_time_out(size_t time) {
    recv_time_out.store(time);
    this->recv_timer.expires_after(std::chrono::seconds(time));
}

void tcp_session::set_recv_buffer_size(size_t size) {
    asio::socket_base::receive_buffer_size recv_buf_size(size);
    super_type::set_option(recv_buf_size);
}

void tcp_session::set_send_buffer_size(size_t size) {
    asio::socket_base::send_buffer_size send_buf_size(size);
    super_type::set_option(send_buf_size);
}

void tcp_session::set_send_low_water_mark(size_t size) {
    asio::socket_base::send_low_watermark slw(size);
    super_type::set_option(slw);
}

void tcp_session::set_recv_low_water_mark(size_t size) {
    asio::socket_base::receive_low_watermark rlw(size);
    super_type::set_option(rlw);
}

void tcp_session::set_no_delay(bool no_delay) {
    asio::ip::tcp::no_delay _no_delay(no_delay);
    super_type::set_option(_no_delay);
}

void tcp_session::send(basic_buffer<char> &buffer) {
    if (_buffer.empty())
        _buffer.swap(buffer);
    else
        _buffer.append(buffer.data(), buffer.size());
    return this->write_l();
}

void tcp_session::send(const char *data, size_t length) {
    basic_buffer<char> tmp(data, length);
    return send(tmp);
}

void tcp_session::send(std::string &str) {
    basic_buffer<char> tmp(std::move(str));
    return send(tmp);
}

bool tcp_session::is_server() const {
    return _is_server;
}

void tcp_session::begin_session() {
    Trace("begin tcp session");
    before_begin_session();
    return this->read_l();
}

void tcp_session::read_l() {
    auto stronger_self = std::static_pointer_cast<tcp_session>(shared_from_this());
    auto time_out = recv_time_out.load(std::memory_order_relaxed);
    if (time_out) {
        auto origin_time_out = clock_type::now() + std::chrono::seconds(time_out);
        recv_timer.expires_at(origin_time_out);
        recv_timer.template async_wait([stronger_self, origin_time_out](const std::error_code &e) {
            Trace(e.message());
            //此时只剩定时器持有引用
            if (stronger_self.unique() || e) {
                return;
            }
            Error("session receive timeout");
            stronger_self->shutdown(shutdown_receive);
        });
    }
    auto read_function = [stronger_self](const std::error_code &e, size_t length) {
        if (e) {
            stronger_self->onError(e);
            stronger_self->recv_timer.cancel();
            if (stronger_self->_is_server)
                get_session_helper().remove_session(stronger_self);
            return;
        }
        stronger_self->onRecv(stronger_self->buffer, length);
        stronger_self->read_l();
    };
    std::shared_ptr<basic_session> session_ptr(shared_from_this());
    async_read_some(asio::buffer(stronger_self->buffer, 10240), read_function);
}

void tcp_session::write_l() {
    auto stronger_self = std::static_pointer_cast<tcp_session>(shared_from_this());
    auto time_out = send_time_out.load(std::memory_order_relaxed);
    if (time_out) {
        auto origin_time_out = clock_type::now() + std::chrono::seconds(time_out);
        send_timer.expires_at(origin_time_out);
        send_timer.template async_wait([stronger_self](const std::error_code &e) {
            //此时只剩定时器持有引用
            if (stronger_self.unique() || e) {
                return;
            }
            Error("session send timeout");
            stronger_self->super_type::shutdown(shutdown_both);
        });
    }
    auto write_function = [stronger_self](const std::error_code &ec, size_t send_length) {
        if (ec) {
            stronger_self->send_timer.cancel();
            stronger_self->shutdown(shutdown_both);
            return;
        }
        stronger_self->_buffer.remove(send_length);
        if (!stronger_self->_buffer.empty()) {
            return stronger_self->write_l();
        } else {
            //如果没有数据可发，取消定时器
            stronger_self->send_timer.cancel();
        }
    };
    async_write_some(asio::buffer(_buffer.data(), _buffer.size()), write_function);
}