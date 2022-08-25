/*
* @file_name: srt_client.cpp
* @date: 2022/08/09
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "srt_client.hpp"
#include "spdlog/logger.hpp"
#include "srt_error.hpp"
namespace srt {
    class srt_client::impl : public srt_socket_service {
    public:
        impl(asio::io_context &poller, const endpoint_type &host) : poller(poller), srt_socket_service(poller), _sock(poller) {
            _sock.open(host.protocol());
            _sock.bind(host);
            _sock.native_non_blocking(true);
            asio::socket_base::receive_buffer_size rbs(256 * 1024);
            asio::socket_base::send_buffer_size sbs(256 * 1024);
            _sock.set_option(rbs);
            _sock.set_option(sbs);
            this->host = host;
            receive_cache = std::make_shared<buffer>();
            receive_cache->resize(1500);
            conn_func = [](const std::error_code &e) {};
            err_func = conn_func;
        }

        const asio::ip::udp::endpoint &get_remote_endpoint() override {
            return this->remote;
        }

        const asio::ip::udp::endpoint &get_local_endpoint() override {
            return this->host;
        }

        void set_on_error(const std::function<void(const std::error_code &)> &f) {
            this->err_func = f;
        }

        void async_connect(const endpoint_type &_remote, const std::function<void(const std::error_code &e)> &f) {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            poller.post([self, _remote, f]() {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                stronger_self->conn_func = f;
                stronger_self->remote = _remote;
                return stronger_self->connect_self();
            });
        }

        int async_send(const char *data, size_t length) {
            if (flag.load(std::memory_order_relaxed)) {
                return -1;
            }
            return srt_socket_service::async_send(buffer::assign(data, length));
        }

    protected:
        void send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) override {
#if 1
            try {
                auto ret = _sock.send(asio::buffer(buff->data(), buff->size()));
            } catch (const std::system_error &e) {
                return on_error_in(e.code());
            }
#else
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            _sock.async_send(asio::buffer(buff->data(), buff->size()), [self, buff](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }

                if (!e) {
                    return;
                }
                bool is_call = false;
                if (stronger_self->flag.compare_exchange_weak(is_call, true)) {
                    return stronger_self->on_error_in(e);
                }
            });
#endif
        }

        void on_connected() override {
            is_connect_func = true;
            auto c = srt::make_srt_error(success);
            return conn_func(c);
        }

        void on_error(const std::error_code &e) override {
            Error(e.message());
            if (!is_connect_func) {
                is_connect_func = true;
                return conn_func(e);
            }
            return err_func(e);
        }

    public:
        void begin() override {
            srt_socket_service::begin();
        }

    private:
        void connect_self() {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            endpoint_type _tmp_endpoint = remote;
            _sock.async_connect(_tmp_endpoint, [self, _tmp_endpoint](const std::error_code &e) {
                if (auto stronger_self = self.lock()) {
                    if (e) {
                        return stronger_self->on_error_in(e);
                    } else {
                        /// begin to read
                        stronger_self->reading();
                        return stronger_self->connect();
                    }
                }
            });
        }
        void reading() {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            _sock.async_receive(asio::buffer((char *) receive_cache->data(), receive_cache->size()), [self](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }

                if (stronger_self->flag.load(std::memory_order_relaxed)) {
                    return;
                }

                if (e) {
                    return;
                }

                if (length <= 0) {
                    return;
                }
                stronger_self->receive_cache->resize(length);
                stronger_self->receive_cache->backward();
                stronger_self->input_packet(stronger_self->receive_cache);
                /// 恢复之
                stronger_self->receive_cache->backward();
                stronger_self->receive_cache->resize(1500);
                stronger_self->reading();
            });
        }

    private:
        std::shared_ptr<buffer> receive_cache;
        asio::ip::udp::socket _sock;
        asio::io_context &poller;
        std::atomic<bool> flag{false};
        bool is_connect_func = false;
        endpoint_type host;
        endpoint_type remote;
        std::function<void(const std::error_code &)> conn_func;
        std::function<void(const std::error_code &)> err_func;
    };
};// namespace srt


namespace srt {

    srt_client::srt_client(asio::io_context &poller, const endpoint_type &host) {
        _impl = std::make_shared<srt_client::impl>(poller, host);
        _impl->begin();
    }

    void srt_client::async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &e)> &f) {
        _impl->async_connect(remote, f);
    }
    void srt_client::set_on_error(const std::function<void(const std::error_code &)> &f) {
        return _impl->set_on_error(f);
    }
    int srt_client::async_send(const char *data, size_t length) {
        return _impl->async_send(data, length);
    }
};// namespace srt
