/*
* @file_name: stun_server.hpp
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

#ifndef TOOLKIT_STUN_SERVER_H
#define TOOLKIT_STUN_SERVER_H
#include <net/asio.hpp>
namespace stun {
    namespace udp {
        /// The STUN server MUST support the Binding method. It SHOULD NOT
        /// utilize the short-term or long-term credential mechanism. This is
        /// because the work involved in authenticating the request is more than
        /// the work in simply processing it. It SHOULD NOT utilize the
        /// ALTERNATE-SERVER mechanism for the same reason. It MUST support UDP
        /// and TCP. It MAY support STUN over TCP/TLS; however, TLS provides
        /// minimal security benefits in this basic mode of operation. It MAY
        /// utilize the FINGERPRINT mechanism but MUST NOT require it. Since the
        /// stand-alone server only runs STUN, FINGERPRINT provides no benefit.
        /// Requiring it would break compatibility with RFC 3489, and such
        /// compatibility is desirable in a stand-alone server. Stand-alone STUN
        /// servers SHOULD support backwards compatibility with [RFC3489]
        /// clients, as described in Section 12.
        class stun_server : public std::enable_shared_from_this<stun_server> {

        };
    };// namespace udp


    std::shared_ptr<udp::stun_server> create_server(uint16_t port = 3489, const std::string &host = "0.0.0.0");
};// namespace stun


#endif//TOOLKIT_STUN_SERVER_H
