/*
* @file_name: stun_address.cpp
* @date: 2022/08/03
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

#include "stun_address.h"
#include "net/asio.hpp"
#include "net/buffer.hpp"
#include "stun_error.h"
#include "stun_error_code.h"
#include "stun_packet.h"
#include <algorithm>
#include <array>
namespace stun {
    /// The MAPPED-ADDRESS attribute indicates a reflexive transport address
    /// of the client.
    /// It consists of an 8-bit address family and a 16-bit
    /// port, followed by a fixed-length value representing the IP address.
    /// If the address family is IPv4, the address MUST be 32 bits. If the
    /// address family is IPv6, the address MUST be 128 bits. All fields
    /// must be in network byte order.
    const uint16_t ipv4_family = 0x01;
    const uint16_t ipv6_family = 0x02;
    void put_mapped_address_or_alternate_server(const attributes &a, const std::shared_ptr<buffer> &buf, const std::string &ip, uint16_t port) {
        if (a != attributes::mapped_address && a != attributes::alternate_server) {
            throw std::system_error(stun::make_stun_error(stun::attribute_is_invalid, generate_stun_packet_category()));
        }
        buffer b;
        auto ip_address = asio::ip::make_address(ip);
        uint16_t family = ip_address.is_v4() ? ipv4_family : ipv6_family;
        /// family
        b.put_be<uint16_t>(family);
        b.put_be<uint16_t>(port);
        if (ip_address.is_v4()) {
            auto v4 = ip_address.to_v4().to_bytes();
            b.append(v4.begin(), v4.end());
        } else {
            auto v6 = ip_address.to_v6().to_bytes();
            b.append(v6.begin(), v6.end());
        }
        attribute_type attr;
        attr.attribute = a;
        attr.value.assign(b.begin(), b.end());
        attr.length = attr.value.size();
        stun_add_attribute(buf, attr);
    }
    /// X-Port is computed by taking the mapped port in host byte order,
    /// XOR’ing it with the most significant 16 bits of the magic cookie, and
    /// then the converting the result to network byte order.
    ///
    /// If the IP address family is IPv4, X-Address is computed by taking the mapped IP
    /// address in host byte order, XOR’ing it with the magic cookie, and
    /// converting the result to network byte order.
    ///
    /// If the IP address family is IPv6, X-Address is computed by taking the mapped IP address
    /// in host byte order, XOR’ing it with the concatenation of the magic
    /// cookie and the 96-bit transaction ID, and converting the result to
    /// network byte order.
    void put_xor_mapped_address(const std::shared_ptr<buffer> &buf, const std::string &transaction_id, const std::string &ip, uint16_t port) {
        constexpr uint16_t xor_ing = static_cast<uint32_t>(stun_packet::magic_cookie) >> 16;
        buffer b;

        auto ip_address = asio::ip::make_address(ip);
        uint16_t family = ip_address.is_v4() ? ipv4_family : ipv6_family;
        /// family
        b.put_be<uint16_t>(family);
        /// x-port network byte order
        b.put_be<uint16_t>(port ^ static_cast<uint16_t>(xor_ing));
        /// ipv4
        if (ip_address.is_v4()) {
            /// get the host byte
            auto v4 = ip_address.to_v4().to_uint() ^ stun_packet::magic_cookie;
            b.put_be<uint32_t>(v4);
        } else {
            /// xor'ing
            buffer xor_value;
            xor_value.put_be<uint32_t>(stun_packet::magic_cookie);
            xor_value.append(transaction_id.begin(), transaction_id.end());
            /// ipv6 net byte order
            auto v6 = ip_address.to_v6().to_bytes();
            const char *pointer = xor_value.data();
            std::for_each(v6.rbegin(), v6.rend(), [&](unsigned char &ch) {
                ch = ch ^ (*pointer);
                ++pointer;
            });
            b.append(v6.begin(), v6.end());
        }

        attribute_type attr;
        attr.attribute = attributes::xor_mapped_address;
        attr.value.append(b.data(), b.size());
        attr.length = attr.value.size();
        stun_add_attribute(buf, attr);
    }
};// namespace stun