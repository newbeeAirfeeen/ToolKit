/*
* @file_name: stun_request.h
* @date: 2022/07/29
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

#ifndef TOOLKIT_STUN_REQUEST_HPP
#define TOOLKIT_STUN_REQUEST_HPP
#include <net/asio.hpp>
namespace stun {
    namespace udp {
        class stun_request : public std::enable_shared_from_this<stun_request> {
            friend std::shared_ptr<stun_request> create_request(asio::ip::udp::socket &sock, const std::string &address, uint16_t port);

        public:
            using resolver = asio::ip::udp::resolver;
            using query_type = asio::ip::udp::resolver::query;
            using socket_type = asio::ip::udp::socket;
            using clock_type = std::chrono::system_clock;

        public:
            /// a client SHOULD retransmit a STUN request message starting with an interval
            /// of RTO
            void setRTO(uint16_t rto);

        private:
            stun_request(socket_type &sock, const query_type &query);

        private:
            /// if the success response contains unknown comprehension-required
            /// attributes, the response is discarded
            void process_success_response();
            /// If the error response contains unknown comprehension-required
            /// attributes, or if the error response does not contain an ERROR-CODE
            /// attribute, then the transaction is simply considered to have failed.
            void process_error_response();

        private:
            std::shared_ptr<socket_type> _sock;
            /// initial value for RTO SHOULD be greater than 500ms
            uint16_t _rto = 500;
            query_type query;
            resolver resolver_;
            asio::basic_waitable_timer<clock_type> timer;
        };

        std::shared_ptr<stun_request> create_request(asio::ip::udp::socket &sock, const std::string &address, uint16_t port = 3489);
    };// namespace udp


};// namespace stun


#endif//TOOLKIT_STUN_REQUEST_HPP
