#include "srt_session.hpp"
#include "spdlog/logger.hpp"
#include "srt_error.hpp"
#include "srt_server.hpp"
namespace srt {
    srt_session::srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, asio::io_context &context) : _sock(*_sock), srt_socket_service(context) {
        _local = _sock->local_endpoint();
    }

    srt_session::~srt_session() {
        Warn("~srt session");
    }

    void srt_session::set_parent(const std::shared_ptr<srt_server> &serv) {
        this->_parent_server = serv;
    }

    void srt_session::set_current_remote_endpoint(const asio::ip::udp::endpoint &p) {
        this->_remote = p;
    }

    void srt_session::set_cookie(uint32_t cookie) {
        this->cookie_ = cookie;
    }

    void srt_session::begin_session() {
        std::weak_ptr<srt_session> self(std::static_pointer_cast<srt_session>(srt_socket_service::shared_from_this()));
        /// 设置服务端握手
        srt_socket_service::connect_as_server();
        srt_socket_service::begin();
    }

    void srt_session::receive(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        return srt_socket_service::input_packet(pkt, buff);
    }

    void srt_session::on_session_timeout() {
        auto server = _parent_server.lock();
        if (!server) {
            return;
        }
        server->remove_cookie_session(cookie_);
    }
    const asio::ip::udp::endpoint &srt_session::get_remote_endpoint() {
        return _remote;
    }

    const asio::ip::udp::endpoint &srt_session::get_local_endpoint() {
        return _local;
    }

    void srt_session::onRecv(const std::shared_ptr<buffer> &buff) {
        Info("receive: {}", buff->data());
    }

    void srt_session::onError(const std::error_code &e) {
        Error(e.message());
    }

    void srt_session::on_connected() {
        auto server = _parent_server.lock();
        if (!server) {
            return;
        }
        server->remove_cookie_session(cookie_);
        server->add_connected_session(std::static_pointer_cast<srt_session>(srt_socket_service::shared_from_this()));
    }

    void srt_session::send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) {
#if 0
//        std::weak_ptr<srt_session> self(std::static_pointer_cast<srt_session>(srt_socket_service::shared_from_this()));
//        _sock.async_send_to(asio::buffer(buff->data(), buff->size()), _remote, [self](const std::error_code &e, size_t length) {
//            auto stronger_self = self.lock();
//            if (!stronger_self) {
//                return;
//            }
//            if (e) {
//                return stronger_self->on_error_in(e);
//            }
//        });
#else
        try {
            if (!_sock.native_non_blocking()) {
                _sock.native_non_blocking(true);
            }
            auto ret = _sock.send_to(asio::buffer(buff->data(), buff->size()), where);
        } catch (const std::system_error &e) {
            return on_error_in(e.code());
        }
#endif
    }

    void srt_session::on_error(const std::error_code &e) {
        if (e.value() == srt_error_code::socket_connect_time_out) {
            return on_session_timeout();
        }
        auto server = _parent_server.lock();
        if (!server) {
            return;
        }
        server->remove_session(get_sock_id());
        return onError(e);
    }

    uint32_t srt_session::get_cookie() {
        return cookie_;
    }
};// namespace srt
