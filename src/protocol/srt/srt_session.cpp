#include "srt_session.hpp"
#include "srt_server.hpp"
namespace srt {
    srt_session::srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, asio::io_context &context) : _sock(*_sock), context(context), timer(context) {
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

    asio::io_context &srt_session::get_poller() {
        return context;
    }

    void srt_session::set_max_receive_time_out(uint32_t ms) {
    }

    void srt_session::begin_session() {
        std::weak_ptr<srt_session> self(shared_from_this());
        timer.expires_at(std::chrono::steady_clock::now() + std::chrono::seconds(10));
        timer.async_wait([self](const std::error_code &e) {
            if (e) {
                return;
            }
            if (auto stronger_self = self.lock()) {
                return stronger_self->on_session_timeout();
            }
        });
    }

    void srt_session::receive(const std::shared_ptr<buffer> &buff) {
        last_receive_time_point = std::chrono::steady_clock::now();
        Info("{}", buff->data());
    }

    void srt_session::on_session_timeout() {
        auto server = _parent_server.lock();
        if (!server) {
            return;
        }
        server->remove(_remote);
    }
};// namespace srt
