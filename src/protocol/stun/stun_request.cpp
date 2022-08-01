//
// Created by OAHO on 2022/7/30.
//
#include "stun_request.h"

namespace stun {
    namespace udp {

        stun_request::stun_request(socket_type &sock, const query_type &query) : resolver_(sock.get_executor()), query(query), timer(sock.get_executor()),
                                                                                 _sock(std::make_shared<asio::ip::udp::socket>(std::move(sock))) {}

        void stun_request::setRTO(uint16_t rto) {
            this->_rto = rto;
        }

        void stun_request::process_success_response() {
        }

        void stun_request::process_error_response() {
        }


        std::shared_ptr<stun_request> create_request(asio::ip::udp::socket &sock, const std::string &address, uint16_t port) {
            asio::ip::udp::resolver::query query_(address, std::to_string(port));
            std::shared_ptr<stun_request> request(new stun_request(std::ref(sock), query_));
            return request;
        }
    }// namespace udp
}// namespace stun