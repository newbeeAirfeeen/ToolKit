/*
* @file_name: session_helper.hpp
* @date: 2022/04/04
* @author: oaho
* Copyright @ hz oaho, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#ifndef TOOLKIT_SOCKET_HELPER_HPP
#define TOOLKIT_SOCKET_HELPER_HPP
#include "asio.hpp"
#include "buffer_list.hpp"
#include "event_poller.hpp"


template<typename socket_type, typename sub_type>
class socket_helper;

template<typename sub_type>
class socket_helper<asio::ip::tcp::socket, sub_type> {
public:
    using endpoint_type = typename asio::ip::tcp::socket::endpoint_type;
public:
    template<typename...Args>
    explicit socket_helper(event_poller& poller, Args&&...args):poller(poller),sock_pointer(new asio::ip::tcp::socket(poller.get_executor())),
                                        send_timer(poller.get_executor()),
                                        recv_timer(poller.get_executor()){}
    template<typename...Args>
    socket_helper(event_poller &poller, asio::ip::tcp::socket& sock, Args&&...args):sock_pointer(new asio::ip::tcp::socket(std::move(sock))),
                                        poller(poller),
                                        send_timer(poller.get_executor()),
                                        recv_timer(poller.get_executor()) {}

    virtual std::shared_ptr<sub_type> shared_from_this_sub_type() = 0;
    virtual void onRecv(buffer &buff) = 0;
    virtual void onError(const std::error_code &e) = 0;

    void begin_read() {
        auto stronger_self(shared_from_this_sub_type());
        return poller.template async([stronger_self]() { stronger_self->read_l(); });
    }

    virtual void async_send(buffer &buf) {
        {
            std::lock_guard<decltype(mtx)> lmtx(mtx);
            L2_cache_.emplace_back(std::move(buf));
        }
        auto stronger_self = std::static_pointer_cast<socket_helper<asio::ip::tcp::socket, sub_type>>(shared_from_this_sub_type());
        return poller.async([stronger_self]() { stronger_self->write_l(); });
    }


public:
    void clear() {
        sock_pointer->cancel();
        L1_cache_.clear();
        L2_cache_.clear();
    }

    void set_recv_time_out(size_t time) {
        recv_timer_out.store(time, std::memory_order_relaxed);
        send_timer.expires_after(std::chrono::seconds(time));
    }


    void set_send_time_out(size_t time) {
        send_timer_out.store(time, std::memory_order_relaxed);
        send_timer.expires_after(std::chrono::seconds(time));
    }

protected:
    asio::ip::tcp::socket& getSock(){
        return std::ref(*sock_pointer);
    }

private:
    void read_l() {
        auto stronger_self = shared_from_this_sub_type();
        auto time_out = recv_timer_out.load(std::memory_order_relaxed);
        if (time_out) {
            auto origin_time_out = std::chrono::system_clock::now() + std::chrono::seconds(time_out);
            recv_timer.expires_at(origin_time_out);
            recv_timer.template async_wait([stronger_self, origin_time_out](const std::error_code &e) {
                Trace(e.message());
                //此时只剩定时器持有引用
                if (stronger_self.unique() || e) {
                    return;
                }
                Error("session receive timeout");
                stronger_self->sock_pointer->shutdown(asio::socket_base::shutdown_receive);
            });
        }
        auto read_function = [stronger_self](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                stronger_self->recv_timer.cancel();
                return;
            }
            stronger_self->recv_cache.resize(length);
            stronger_self->onRecv(stronger_self->recv_cache);
            stronger_self->read_l();
        };
        recv_cache.backward();
        recv_cache.resize(10 * 1024);
        sock_pointer->async_read_some(asio::buffer(recv_cache.data(), recv_cache.size()), read_function);
    }
    //此函数会在poller绑定的线程执行
    void write_l() {
        auto stronger_self = std::static_pointer_cast<socket_helper<asio::ip::tcp::socket, sub_type>>(shared_from_this_sub_type());
        //在自己绑定的线程执行
        if (!stronger_self->is_sending) {
            stronger_self->is_sending = true;
            //如果设置失败
            {
                std::lock_guard<std::recursive_mutex> lmtx(stronger_self->mtx);
                stronger_self->L1_cache_.splice(stronger_self->L1_cache_.end(), stronger_self->L2_cache_);
                if (stronger_self->L1_cache_.empty()) {
                    stronger_self->is_sending = false;
                    return;
                }
            }
            auto time_out = send_timer_out.load(std::memory_order_relaxed);
            if (time_out) {
                auto origin_time_out = std::chrono::system_clock::now() + std::chrono::seconds(time_out);
                stronger_self->send_timer.expires_at(origin_time_out);
                stronger_self->send_timer.async_wait([stronger_self](const std::error_code &e) {
                    //此时只剩定时器持有引用
                    if (stronger_self.unique() || e) {
                        return;
                    }
                    Error("session send timeout");
                    stronger_self->sock_pointer->shutdown(asio::socket_base::shutdown_both);
                });
            }
            auto write_func = [stronger_self](const std::error_code &e, size_t length) {
                if (stronger_self.unique()) {
                    return;
                }
                if (e) {
                    stronger_self->is_sending = false;
                    return stronger_self->onError(e);
                }
                stronger_self->L1_cache_.remove(length);
                stronger_self->is_sending = false;
                if (stronger_self->L1_cache_.total_buffer_size()) {
                    return stronger_self->write_l();
                }
                else{
                    stronger_self->send_timer.cancel();
                }
            };
            stronger_self->sock_pointer->async_send(stronger_self->L1_cache_, write_func);
        }
    }


private:
    event_poller &poller;
    std::recursive_mutex mtx;
    std::unique_ptr<asio::ip::tcp::socket> sock_pointer;
    asio::basic_waitable_timer<std::chrono::system_clock> send_timer;
    asio::basic_waitable_timer<std::chrono::system_clock> recv_timer;
    /**
     * 接收缓冲区
     */
    mutable_basic_buffer<char> recv_cache;
    /**
     * 一级发送缓存
     */
    basic_buffer_list<char> L1_cache_;
    /**
     * 二级发送缓存
     */
    basic_buffer_list<char> L2_cache_;
    /**
     * 是否已经在发送
     */
    bool is_sending = false;
    std::atomic<size_t> recv_timer_out{0};
    std::atomic<size_t> send_timer_out{0};
};

template<typename sub_type>
class socket_helper<asio::ip::udp::socket, sub_type> {
public:
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;

public:
    template<typename...Args>
    socket_helper(event_poller& poller, asio::ip::udp::socket& sock, Args&&...args):poller(poller),sock_pointer(new asio::ip::udp::socket(std::move(sock))){}

    template<typename...Args>
    explicit socket_helper(event_poller &poller, Args&&...args): sock_pointer(new asio::ip::udp::socket(poller.get_executor())),poller(poller) {}

    virtual std::shared_ptr<sub_type> shared_from_this_sub_type() = 0;

    virtual void onRecv(buffer &buff, const endpoint_type &endpoint) = 0;

    virtual void onError(const std::error_code &e, const endpoint_type &endpoint) = 0;

    void begin_read() {
        auto stronger_self(shared_from_this_sub_type());
        return poller.template async([stronger_self]() { stronger_self->read_l(); });
    }

    void async_send(buffer &buf, const endpoint_type &endpoint) {
        auto stronger_self(shared_from_this_sub_type());
        std::shared_ptr<buffer> save_buf = std::make_shared<buffer>(std::move(buf));
        auto write_func = [stronger_self, endpoint, save_buf](const std::error_code &e, size_t length) {
            if (stronger_self.unique()) {
                return;
            }
            if (e) {
                return stronger_self->onError(e, endpoint);
            }
        };
        return sock_pointer->template async_send_to(asio::buffer(save_buf->data(), save_buf->size()), endpoint, write_func);
    }
protected:
    asio::ip::udp::socket & getSock(){
        return std::ref(*sock_pointer);
    }
private:
    void read_l() {
        auto stronger_self = shared_from_this_sub_type();
        auto endpoint = std::make_shared<endpoint_type>();
        auto read_function = [stronger_self, endpoint](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                stronger_self->recv_timer.cancel();
                return;
            }
            stronger_self->recv_cache.resize(length);
            stronger_self->onRecv(stronger_self->recv_cache, *endpoint);
            stronger_self->read_l();
        };
        recv_cache.backward();
        recv_cache.resize(2048);
        sock_pointer->template async_receive_from(recv_cache, *endpoint, read_function);
    }

public:
    void clear() {
        sock_pointer->cancel();
        recv_cache.clear();
    }

private:
    event_poller &poller;
    std::unique_ptr<asio::ip::udp::socket> sock_pointer;
    mutable_basic_buffer<char> recv_cache;
};


#ifdef SSL_ENABLE
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
template<typename sub_type>
class socket_helper<asio::ssl::stream<asio::ip::tcp::socket>, sub_type> {
public:
    using endpoint_type = typename asio::ip::tcp::socket::endpoint_type;
public:
    socket_helper(event_poller& poller, const std::shared_ptr<asio::ssl::context> &ssl_context, asio::ip::tcp::socket& sock, bool is_server = true)
        :sock_pointer(new asio::ssl::stream<asio::ip::tcp::socket>(std::move(sock), *ssl_context)),
          poller(poller),
          send_timer(poller.get_executor()),
          recv_timer(poller.get_executor()),
          ssl_context(ssl_context),
          is_server(is_server){}
    socket_helper(event_poller& poller, const std::shared_ptr<asio::ssl::context> &ssl_context, bool is_server = true)
        :sock_pointer(new asio::ssl::stream<asio::ip::tcp::socket>(poller.get_executor(), *ssl_context)),
          poller(poller),
          send_timer(poller.get_executor()),
          recv_timer(poller.get_executor()),
          ssl_context(ssl_context),
          is_server(is_server) {}

    virtual std::shared_ptr<sub_type> shared_from_this_sub_type() = 0;
    virtual void onRecv(buffer &buff) = 0;
    virtual void onError(const std::error_code &e) = 0;

    void begin_read() {
        auto stronger_self(shared_from_this_sub_type());
        return poller.template async([stronger_self]() {
            stronger_self->handshake([stronger_self]() {
                stronger_self->read_l();
            });
        });
    }

    void async_send(buffer &buf) {
        if (!is_handshake) {
            auto stronger_self(shared_from_this_sub_type());
            std::shared_ptr<buffer> buffers = std::make_shared<buffer>(std::move(buf));
            return handshake([stronger_self, buffers]() {
                return stronger_self->async_send(*buffers);
            });
        }
        {
            std::lock_guard<decltype(mtx)> lmtx(mtx);
            L2_cache_.template emplace_back(std::move(buf));
        }
        auto stronger_self(shared_from_this_sub_type());
        return poller.template async([stronger_self]() { stronger_self->write_l(); });
    }


public:
    void clear() {
        sock_pointer->next_layer().cancel();
        L1_cache_.clear();
        L2_cache_.clear();
    }

    void set_recv_time_out(size_t time) {
        recv_timer_out.store(time, std::memory_order_relaxed);
        send_timer.expires_after(std::chrono::seconds(time));
    }


    void set_send_time_out(size_t time) {
        send_timer_out.store(time, std::memory_order_relaxed);
        send_timer.expires_after(std::chrono::seconds(time));
    }
protected:
    asio::ip::tcp::socket & getSock(){
        return std::ref(sock_pointer->next_layer());
    }
private:
    template<typename Func, typename... Args>
    void handshake(Func &&func, Args &&...args) {
        if (is_handshake) {
            return read_l();
        }
        auto stronger_self(shared_from_this_sub_type());
        auto cb = std::bind(func, std::forward<Args>(args)...);
        auto handshake_func = [stronger_self, cb](const std::error_code &e) {
            if (e) {
                return stronger_self->onError(e);
            }
            stronger_self->is_handshake = true;
            cb();
        };
        sock_pointer->template async_handshake(is_server ? asio::ssl::stream_base::handshake_type::server
                                                : asio::ssl::stream_base::handshake_type::client,
                                      handshake_func);
    }
    void read_l() {
        auto stronger_self = shared_from_this_sub_type();
        auto time_out = recv_timer_out.load(std::memory_order_relaxed);
        if (time_out) {
            auto origin_time_out = std::chrono::system_clock::now() + std::chrono::seconds(time_out);
            recv_timer.expires_at(origin_time_out);
            recv_timer.template async_wait([stronger_self, origin_time_out](const std::error_code &e) {
                Trace(e.message());
                //此时只剩定时器持有引用
                if (stronger_self.unique() || e) {
                    return;
                }
                Error("session receive timeout");
                stronger_self->sock_pointer->next_layer().shutdown(asio::socket_base::shutdown_receive);
            });
        }
        auto read_function = [stronger_self](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                stronger_self->recv_timer.cancel();
                return;
            }
            stronger_self->recv_cache.resize(length);
            stronger_self->onRecv(stronger_self->recv_cache);
            stronger_self->read_l();
        };
        recv_cache.backward();
        recv_cache.resize(10 * 1024);
        sock_pointer->async_read_some(recv_cache, read_function);
    }
    //此函数会在poller绑定的线程执行
    void write_l() {
        auto stronger_self = std::static_pointer_cast<socket_helper<asio::ssl::stream<asio::ip::tcp::socket>, sub_type>>(shared_from_this_sub_type());
        //在自己绑定的线程执行
        if (!is_sending) {
            is_sending = true;
            //如果设置失败
            {
                std::lock_guard<std::recursive_mutex> lmtx(stronger_self->mtx);
                stronger_self->L1_cache_.splice(stronger_self->L1_cache_.end(), stronger_self->L2_cache_);
                if (stronger_self->L1_cache_.empty()) {
                    stronger_self->is_sending = false;
                    return;
                }
            }
            auto time_out = stronger_self->recv_timer_out.load(std::memory_order_relaxed);
            if (time_out) {
                auto origin_time_out = std::chrono::system_clock::now() + std::chrono::seconds(time_out);
                stronger_self->send_timer.expires_at(origin_time_out);
                stronger_self->send_timer.template async_wait([stronger_self](const std::error_code &e) {
                    //此时只剩定时器持有引用
                    if (stronger_self.unique() || e) {
                        return;
                    }
                    Error("session send timeout");
                    stronger_self->sock_pointer->next_layer().shutdown(asio::socket_base::shutdown_both);
                });
            }
            auto write_func = [stronger_self](const std::error_code &e, size_t length) {
                if (stronger_self.unique()) {
                    return;
                }
                if (e) {
                    stronger_self->is_sending = false;
                    return stronger_self->onError(e);
                }
                stronger_self->L1_cache_.remove(length);
                stronger_self->is_sending = false;
                if (stronger_self->L1_cache_.total_buffer_size()) {
                    return stronger_self->write_l();
                }
                else{
                    stronger_self->send_timer.cancel();
                }
            };
            stronger_self->sock_pointer->async_write_some(stronger_self->L1_cache_, write_func);
        }
    }


private:
    event_poller &poller;
    std::recursive_mutex mtx;
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sock_pointer;
    asio::basic_waitable_timer<std::chrono::system_clock> send_timer;
    asio::basic_waitable_timer<std::chrono::system_clock> recv_timer;
    /**
     * 接收缓冲区
     */
    mutable_basic_buffer<char> recv_cache;
    /**
     * 一级发送缓存
     */
    basic_buffer_list<char> L1_cache_;
    /**
     * 二级发送缓存
     */
    basic_buffer_list<char> L2_cache_;
    /**
     * 是否已经在发送
     */
    bool is_sending = false;
    bool is_handshake = false;
    std::atomic<size_t> recv_timer_out{0};
    std::atomic<size_t> send_timer_out{0};
    std::shared_ptr<asio::ssl::context> ssl_context;
    bool is_server = true;
};

#include <asio/ssl/dtls.hpp>

template<typename sub_type>
class socket_helper<asio::ssl::dtls::socket<asio::ip::udp::socket>, sub_type> {
public:
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;
public:
    socket_helper(event_poller &poller, const std::shared_ptr<asio::ssl::dtls::context>&dtls_context, asio::ip::udp::socket& sock)
        : sock_pointer(new asio::ssl::dtls::socket<asio::ip::udp::socket>(poller.get_executor(), *dtls_context)),
          poller(poller),
          dtls_context(dtls_context){}
    socket_helper(event_poller &poller, const std::shared_ptr<asio::ssl::dtls::context> &dtls_context)
        : sock_pointer(new asio::ssl::dtls::socket<asio::ip::udp::socket>(poller.get_executor(), *dtls_context)),
          poller(poller),
          dtls_context(dtls_context){}

    virtual std::shared_ptr<sub_type> shared_from_this_sub_type() = 0;

    virtual void onRecv(buffer &buff) = 0;

    virtual void onError(const std::error_code &e) = 0;

    void begin_read() {
        auto stronger_self(shared_from_this_sub_type());
        return poller.template async([stronger_self]() { stronger_self->handshake(); });
    }

    void async_send(buffer &buf, const endpoint_type &endpoint) {
        if (!is_handshake) {
            auto stronger_self(shared_from_this_sub_type());
            std::shared_ptr<buffer> buffers = std::make_shared<buffer>(std::move(buf));
            return handshake([stronger_self, buffers]() {
                return stronger_self->async_send(*buffers);
            });
        }
        auto stronger_self(shared_from_this_sub_type());
        std::shared_ptr<buffer> save_buf = std::make_shared<buffer>(std::move(buf));
        auto write_func = [stronger_self, endpoint, save_buf](const std::error_code &e, size_t length) {
            if (stronger_self.unique()) {
                return;
            }
            if (e) {
                return stronger_self->onError(e, endpoint);
            }
        };
        sock_pointer->template async_send(*save_buf, write_func);
    }
protected:
    asio::ip::udp::socket & getSock(){
        return std::ref(sock_pointer->next_layer());
    }

private:
    template<typename Func, typename... Args>
    void handshake(Func &&func, Args &&...args) {
        if (is_handshake) {
            return read_l();
        }
        auto stronger_self(shared_from_this_sub_type());
        auto cb = std::bind(func, std::forward<Args>(args)...);
        auto handshake_func = [stronger_self, cb](const std::error_code &e) {
            if (e) {
                return stronger_self->onError(e);
            }
            stronger_self->is_handshake = true;
            cb();
        };
        sock_pointer->template async_handshake(asio::ssl::stream_base::handshake_type::server, handshake_func);
    }
    void read_l() {
        auto stronger_self = shared_from_this_sub_type();
        auto read_function = [stronger_self](const std::error_code &e, size_t length) {
            if (e) {
                stronger_self->onError(e);
                stronger_self->recv_timer.cancel();
                return;
            }
            stronger_self->recv_cache.resize(length);
            stronger_self->onRecv(stronger_self->recv_cache);
            stronger_self->read_l();
        };
        recv_cache.backward();
        recv_cache.resize(2048);
        sock_pointer->template async_receive(recv_cache, read_function);
    }

public:
    void clear() {
        sock_pointer->next_layer().cancel();
        recv_cache.clear();
        recv_cache.backward();
    }

private:
    event_poller &poller;
    bool is_handshake = false;
    std::unique_ptr<asio::ssl::dtls::socket<asio::ip::udp::socket>> sock_pointer;
    mutable_basic_buffer<char> recv_cache;
    std::shared_ptr<asio::ssl::dtls::context> dtls_context;
};

#endif
#endif//TOOLKIT_SOCKET_HELPER_HPP
