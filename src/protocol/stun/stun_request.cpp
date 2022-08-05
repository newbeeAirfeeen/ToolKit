//
// Created by OAHO on 2022/7/30.
//
#include "stun_request.h"
#include "stun_packet.h"
#include <chrono>
#include <iostream>
namespace stun {
    namespace udp {

        stun_request::stun_request(socket_type &sock, const stun_packet &pkt, const query_type &query) : resolver_(sock.get_executor()), query(query), timer(sock.get_executor()),
                                                                                                         _sock(std::make_shared<asio::ip::udp::socket>(std::move(sock))) {
            request_buff = stun_packet::create_packet(pkt);
            read_buff.resize(1500);
        }

        void stun_request::process_success_response() {
        }

        void stun_request::process_error_response() {
        }

        void stun_request::loop() {
            std::weak_ptr<stun_request> self(shared_from_this());
            try {
                auto address = asio::ip::make_address(this->query.host_name());
                asio::ip::udp::endpoint endpoint(address, std::stoi(query.service_name()));
                connecting(endpoint);
            } catch (...) {
                /// if not direct host , resolve it.
                resolver_.async_resolve(query, [self](const std::error_code &e, const asio::ip::udp::resolver::results_type &result) {
                    auto stronger_self = self.lock();
                    if (!stronger_self) {
                        return;
                    }
                    if (e) {
                        stronger_self->process_error_response();
                        return;
                    }

                    stronger_self->connecting(result->endpoint());
                });
            }
        }


        void stun_request::connecting(const asio::ip::udp::endpoint &endpoint) {
            std::weak_ptr<stun_request> self(shared_from_this());
            _sock->async_connect(endpoint, [self](const std::error_code &e) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                if (e) {
                    return stronger_self->process_error_response();
                }
                stronger_self->begin_to_read();
                stronger_self->on_connected();
            });
        }

        void stun_request::begin_to_read() {
            std::weak_ptr<stun_request> self(shared_from_this());
            _sock->async_receive(asio::buffer(read_buff.data(), read_buff.size()), [self](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                if (e) {
                    std::cerr << e.message() << std::endl;
                    return stronger_self->process_error_response();
                }
                stronger_self->read_buff.resize(length);
                stronger_self->on_data(stronger_self->read_buff);
                stronger_self->read_buff.resize(1500);
                stronger_self->begin_to_read();
            });
        }


        void stun_request::on_connected() {
            std::weak_ptr<stun_request> self(shared_from_this());
            /// 开始发送
            _sock->async_send(asio::buffer(request_buff->data(), request_buff->size()), [self](const std::error_code &e, size_t length) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }
                if (e) {
                    return stronger_self->process_error_response();
                }

                if (length != stronger_self->request_buff->size()) {
                    return stronger_self->process_error_response();
                }
                stronger_self->update_timer();
            });
        }

        void stun_request::update_timer() {
            std::weak_ptr<stun_request> self(shared_from_this());
            timer.expires_after(std::chrono::milliseconds(this->_rto));
            timer.async_wait([self](const std::error_code &e) {
                auto stronger_self = self.lock();
                if (!stronger_self) {
                    return;
                }

                if (e) {
                    return stronger_self->process_error_response();
                }

                if (stronger_self->_rto == 500) {
                    stronger_self->_rto = 1000;
                } else {
                    stronger_self->_rto = (1 << stronger_self->multiple_times) * 1000;
                    ++(stronger_self->multiple_times);
                }
                if (stronger_self->_rto >= 32000)
                    return stronger_self->process_error_response();
                return stronger_self->on_connected();
            });
        }

        void stun_request::on_data(buffer &buf) {
            /// 取消定时器
            this->timer.cancel();
        }


        std::shared_ptr<stun_request> create_request(asio::ip::udp::socket &sock, const stun_packet &pkt, const std::string &address, uint16_t port) {
            asio::ip::udp::resolver::query query_(address, std::to_string(port));
            std::shared_ptr<stun_request> request(new stun_request(std::ref(sock), pkt, query_));
            return request;
        }

        void send_request(const std::shared_ptr<stun_request> &req) {
            req->loop();
        }
    }// namespace udp
}// namespace stun