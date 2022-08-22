#include "srt_session.hpp"
#include "spdlog/logger.hpp"
#include "srt_error.hpp"
#include "srt_server.hpp"
namespace srt {
    srt_session::srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, asio::io_context &context) : _sock(*_sock), srt_socket_service(context) {
        _local = _sock->local_endpoint();
        _pre_handshake_timer = create_deadline_timer<int>(context);
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
        _pre_handshake_timer->set_on_expired([self](const int &v) {
            if (auto stronger_self = self.lock()) {
                stronger_self->on_error_in(make_srt_error(srt_error_code::socket_connect_time_out));
                return stronger_self->on_session_timeout();
            }
        });
        _pre_handshake_timer->add_expired_from_now(10 * 1000, 1);
    }

    void srt_session::receive(const std::shared_ptr<buffer> &buff) {
        return srt_socket_service::input_packet(buff);
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

    void srt_session::on_connected() {
        /// 连接后释放无用的定时器
        _pre_handshake_timer = nullptr;
        auto server = _parent_server.lock();
        if (!server) {
            return;
        }
        server->remove_cookie_session(cookie_);
        server->add_connected_session(std::static_pointer_cast<srt_session>(srt_socket_service::shared_from_this()));
    }

    void srt_session::send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) {
        std::weak_ptr<srt_session> self(std::static_pointer_cast<srt_session>(srt_socket_service::shared_from_this()));
        _sock.async_send_to(asio::buffer(buff->data(), buff->size()), _remote, [self](const std::error_code &e, size_t length) {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            if (e) {
                return stronger_self->on_error_in(e);
            }
        });
    }

    void srt_session::on_error(const std::error_code &e) {
    }
};// namespace srt
