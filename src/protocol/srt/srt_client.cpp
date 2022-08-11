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

namespace srt {
    class srt_client::impl : public srt_socket_service {
    public:
        impl(asio::io_context &poller, const endpoint_type &host) : poller(poller), srt_socket_service(poller), _sock(poller) {
            _sock.open(host.protocol());
            _sock.bind(host);
            this->host = host;
            receive_cache = std::make_shared<buffer>();
            receive_cache->resize(1500);
        }

        const asio::ip::udp::endpoint &get_remote_endpoint() override {
            return this->remote;
        }

        const asio::ip::udp::endpoint &get_local_endpoint() override {
            return this->host;
        }

        void async_connect(const endpoint_type &_remote, const std::function<void(const std::error_code &)> &f) {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            poller.post([self, f, _remote]() {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                stronger_self->connect_func = f;
                stronger_self->remote = _remote;
                return stronger_self->connect();
            });
        }

        void send(const std::shared_ptr<buffer> &buff, const asio::ip::udp::endpoint &where) override {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            _sock.async_send_to(asio::buffer(buff->data(), buff->size()), where, [self, buff](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }

                if (e) {
                    return stronger_self->on_error(e);
                }
            });
        }

        void on_connected() override {
        }

        void on_error(const std::error_code &e) override {
            Error(e.message());
        }

        void begin() override {
            begin_in();
            srt_socket_service::begin();
        }

        void begin_in() {
            std::weak_ptr<impl> self(std::static_pointer_cast<impl>(shared_from_this()));
            _sock.async_receive_from(asio::buffer((char *) receive_cache->data(), receive_cache->size()), other, [self](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                if (stronger_self->other != stronger_self->remote) {
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
                stronger_self->begin_in();
            });
        }

    private:
        std::shared_ptr<buffer> receive_cache;
        asio::ip::udp::socket _sock;
        asio::io_context &poller;
        endpoint_type host;
        endpoint_type remote;
        endpoint_type other;
        std::function<void(const std::error_code &)> connect_func;
    };
};// namespace srt


namespace srt {

    srt_client::srt_client(asio::io_context &poller, const endpoint_type &host) {
        _impl = std::make_shared<srt_client::impl>(poller, host);
        _impl->begin();
    }

    void srt_client::async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &)> &f) {
        return _impl->async_connect(remote, f);
    }
};// namespace srt
