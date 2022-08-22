#include "srt_server.hpp"
#include "Util/endian.hpp"
#include "net/buffer.hpp"
#include "srt_packet.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <memory>
#include <spdlog/logger.hpp>
struct thread_exit_exception : public std::exception {};
class io_context {
public:
    io_context() {
        std::atomic<bool> is_running{false};
        _thread.reset(new std::thread([&]() {
            _id = std::this_thread::get_id();
            is_running.store(true);
            this->run();
        }));
        bool _ = true;
        is_running.compare_exchange_strong(_, true);
    }
    ~io_context() {
        _context.post([]() { throw thread_exit_exception(); });
        if (_thread && _thread->joinable()) _thread->join();
    }
    asio::io_context &get_poller() {
        return std::ref(_context);
    }

    const std::thread::id &get_thread_id() const {
        return this->_id;
    }

private:
    void run() {
        while (true) {
            try {
                asio::executor_work_guard<typename asio::io_context::executor_type> guard(_context.get_executor());
                _context.run();
            } catch (const thread_exit_exception &e) { break; } catch (const std::exception &e) {
            }
        }
    }

private:
    std::unique_ptr<std::thread> _thread;
    asio::io_context _context{1};
    std::thread::id _id;
};

class io_pool {
public:
    ~io_pool() {
        while (!_pool_->empty()) {
            auto *p = _pool_->front();
            if (p) delete p;
            _pool_->pop_front();
        }
    }
    static io_pool &instance() {
        static io_pool pool;
        return pool;
    }

    void for_each(const std::function<void(io_context &)> &f) {
        std::for_each(_pool_->begin(), _pool_->end(), [f](io_context *io) {
            f(*io);
        });
    }

private:
    io_pool() {
        _pool_.reset(new std::list<io_context *>());
        auto size = std::thread::hardware_concurrency();
        size = 4;
        for (decltype(size) i = 0; i < size; i++) {
            _pool_->emplace_back(new io_context);
        }
    }

private:
    std::unique_ptr<std::list<io_context *>> _pool_;
};


namespace srt {
    static thread_local std::unordered_map<uint32_t, std::weak_ptr<srt_session>> _thread_local_session_map_;
    /// handle handshake
    static thread_local decltype(_thread_local_session_map_) _cookie_map;

    void srt_server::start(const asio::ip::udp::endpoint &endpoint) {
        std::weak_ptr<srt_server> self(shared_from_this());
        std::shared_ptr<asio::io_context> _context;
        std::atomic<int> _flag{0};
        auto &pool = io_pool::instance();
        pool.for_each([&, endpoint](io_context &io) {
            io.get_poller().post([&, endpoint, self]() {
                if (auto stronger_self = self.lock()) {
                    auto _sock = stronger_self->create(io.get_poller(), endpoint);
                    {
                        std::lock_guard<std::recursive_mutex> lmtx(stronger_self->mtx);
                        stronger_self->_socks.push_back(_sock);
                    }
                    stronger_self->start(_sock, io.get_poller());
                }
            });
        });
    }

    void srt_server::remove_cookie_session(uint32_t _cookie) {
        _cookie_map.erase(_cookie);
        std::lock_guard<std::recursive_mutex> lmtx(_cookie_mtx);
        _handshake_map.erase(_cookie);
    }

    void srt_server::remove_session(uint32_t sock_id) {
        _thread_local_session_map_.erase(sock_id);
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        _session_map_.erase(sock_id);
        Trace("session map, thread_local size = {}, global size = {}", _thread_local_session_map_.size(), _session_map_.size());
    }

    void srt_server::add_connected_session(const std::shared_ptr<srt_session> &session) {
        /// 同步到线程局部存储
        _thread_local_session_map_.emplace(session->get_sock_id(), session);
        /// 同步到全局会话管理器
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        _session_map_.emplace(session->get_sock_id(), session);
    }

    /// 由各自线程的io_context 调用
    void srt_server::start(const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &context) {
        Info("srt server start on {}:{}", sock->local_endpoint().address().to_string(), sock->local_endpoint().port());
        return read_l(sock, context);
    }

    void srt_server::read_l(const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &context) {
        std::weak_ptr<srt_server> self(shared_from_this());
        std::weak_ptr<asio::ip::udp::socket> sock_self(sock);
        static thread_local auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
        static thread_local auto buff = std::make_shared<buffer>();
        buff->resize(1500);
        buff->backward();
        sock->async_receive_from(asio::buffer((char *) buff->data(), buff->size()), *endpoint, [&, self, sock_self](const std::error_code &e, size_t length) {
            auto stronger_self = self.lock();
            auto sock_stronger_self = sock_self.lock();
            if (!stronger_self || !sock_stronger_self) {
                return;
            }

            if (e) {
                return;
            }

            if (length <= 0) {
                return stronger_self->read_l(sock_stronger_self, context);
            }

            buff->resize(length);
            buff->backward();
            stronger_self->on_receive(buff, *endpoint, sock_stronger_self, context);
            return stronger_self->read_l(sock_stronger_self, context);
        });
    }

    std::shared_ptr<srt_session> srt_server::get_session(uint32_t sock_id) {
        std::lock_guard<std::recursive_mutex> lmtx(mtx);
        auto it = _session_map_.find(sock_id);
        if (it == _session_map_.end()) {
            return nullptr;
        }
        return it->second;
    }

    std::shared_ptr<srt_session> srt_server::get_session_or_create_with_cookie(uint32_t sock_id, bool &ca, const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &poller) {
        ca = false;
        std::lock_guard<std::recursive_mutex> lmtx(_cookie_mtx);
        auto it = _handshake_map.find(sock_id);
        if (it == _handshake_map.end()) {
            /// 表示无此session
            ca = true;
            auto session = std::make_shared<srt_session>(sock, poller);
            _handshake_map.insert(std::make_pair(sock_id, session));
            return session;
        }
        return it->second;
    }

    std::shared_ptr<asio::ip::udp::socket> srt_server::create(asio::io_context &poller, const asio::ip::udp::endpoint &endpoint) {
        auto _sock = std::make_shared<asio::ip::udp::socket>(poller);
        _sock->open(endpoint.protocol());
        asio::ip::udp::socket::reuse_address option(true);
        asio::ip::udp::socket::receive_buffer_size rbs(256 * 1024);
        asio::ip::udp::socket::send_buffer_size sbs(256 * 1024);
        _sock->set_option(option);
        _sock->set_option(rbs);
        _sock->set_option(sbs);
        _sock->bind(endpoint);
        return _sock;
    }

    void srt_server::on_receive(const std::shared_ptr<buffer> &buf, const asio::ip::udp::endpoint &endpoint, const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &poller) {
        try {
            auto pkt = from_buffer(buf->data(), buf->size());
            buf->remove(16);
            /// 新的session握手包
            if (pkt->get_control() && pkt->get_control_type() == control_type::handshake && pkt->get_socket_id() == 0) {
                return handle_handshake(pkt, buf, endpoint, sock, poller);
            }
            return handle_data(pkt, buf);

        } catch (...) {}
    }
    void srt_server::handle_handshake(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &endpoint,
                                      const std::shared_ptr<asio::ip::udp::socket> &sock, asio::io_context &poller) {
        //// 握手包
        if (buff->size() < 48) {
            return;
        }
        auto p = ((const uint32_t *) buff->data()) + 6;
        uint32_t _cookie_ = load_be32(p);
        auto it = _cookie_map.find(_cookie_);
        //// 说明session在本线程进行握手
        if (it != _cookie_map.end()) {
            auto stronger_self = it->second.lock();
            if (!stronger_self) {
                return;
            }
            ///进行握手
            return stronger_self->receive(pkt, buff);
        } else {
            /// 数据漂移到其他线程 或者没有此会话
            bool is_current_thread = false;
            auto session = get_session_or_create_with_cookie(_cookie_, is_current_thread, sock, poller);
            /// 表示是否是新会话
            if (is_current_thread) {
                Debug("create new srt session...");
                /// 同步到线程局部存储
                _cookie_map.insert(std::make_pair(_cookie_, session));
                /// 设置当前cookie
                session->set_cookie(_cookie_);
                session->set_parent(shared_from_this());
                session->set_current_remote_endpoint(endpoint);
                session->begin_session();
                /// 进行握手
                session->receive(pkt, buff);
            }/// 如果不是当前线程.切换到对应线程握手
            else {
                /// 线程局部存储需要拷贝数据
                Warn("data received in other thread, switch to session thread...");
                auto buf_tmp = buffer::assign(buff->data(), buff->size());
                std::weak_ptr<srt_session> session_self(session);
                session->get_poller().post([pkt, buf_tmp, endpoint, session_self]() {
                    if (auto stronger_session_self = session_self.lock()) {
                        stronger_session_self->receive(pkt, buf_tmp);
                    }
                });
            }
        }
    }

    void srt_server::handle_data(const std::shared_ptr<srt_packet> &pkt, const std::shared_ptr<buffer> &buff) {
        //// 大多都在本线程触发，数据漂移到其他线程大概不会频繁调用
        auto it = _thread_local_session_map_.find(pkt->get_socket_id());
        /// 说明是本线程的session, 那么直接调用
        if (it != _thread_local_session_map_.end()) {
            auto stronger = it->second.lock();
            if (stronger) {
                stronger->receive(pkt, buff);
            } else {
                /// 会话被删除,删除弱引用
                _thread_local_session_map_.erase(it);
            }
        }
        /// 数据被其他线程接收或者无此session
        else {
            auto session = get_session(pkt->get_socket_id());
            /// 如果是其他线程的session
            if (!session) {
                return;
            }
            /// 线程局部存储需要拷贝缓冲区
            auto buff_tmp = buffer::assign(buff->data(), buff->size());
            /// 切换到其他线程
            std::weak_ptr<srt_session> session_self(session);
            session->get_poller().post([buff_tmp, session_self, pkt]() {
                if (auto session_stronger = session_self.lock()) {
                    session_stronger->receive(pkt, buff_tmp);
                }
            });
        }
    }
};// namespace srt