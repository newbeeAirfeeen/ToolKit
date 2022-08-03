/*
* @file_name: stun_packet.h
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


#ifndef TOOLKIT_STUN_PACKET_H
#define TOOLKIT_STUN_PACKET_H
#include "net/buffer.hpp"
#include "stun_attributes.h"
#include "stun_method.h"

namespace stun {

    struct stun_packet {
        friend std::shared_ptr<stun_packet> from_buffer(const char *data, size_t length);

    public:
        static constexpr uint32_t magic_cookie = 0x2112A442;

    public:
        static std::shared_ptr<buffer> create_packet(const stun_packet &);

    public:
        void set_method(const stun_method &m);
        void set_finger_print(bool on);
        void set_username(const std::string &username);
        void set_password(const std::string &password);
        void set_realm(const std::string &realm);
        void set_software(const std::string &software);
        void set_unknown_attributes(const std::initializer_list<uint16_t> &);
        void set_alternate_server(const std::string &ip, uint16_t port);
        void set_mapped_address(const std::string &ip, uint16_t port);
        void set_xor_mapped_address(const std::string &ip, uint16_t port);
#ifdef SSL_ENABLE
        void set_message_integrity(bool on);
#endif
        stun_method get_method() const;
        const std::string &get_transaction_id() const;
        const std::string &get_username() const;
        const std::string &get_realm() const;
        const std::string &get_password() const;
        const std::string &get_software() const;
        const std::initializer_list<uint16_t> &get_unknown_attributes() const;
        std::pair<const std::string &, uint16_t> get_alternate_server() const;
        std::pair<const std::string &, uint16_t> get_mapped_address() const;
        std::pair<const std::string &, uint16_t> get_xor_mapped_address() const;

    private:
        stun_method _method = binding_request;
        uint16_t message_length = 0;
        char transaction[12] = {0};

    private:
        bool finger_print = false;
        bool message_integrity = false;
        std::string transaction_id;
        std::string mapped_address;
        std::string xor_mapped_address;
        uint16_t mapped_address_port = 0;
        std::string username;
        std::string password;
        std::string realm;
        std::string software;
        std::string alternate_server;
        uint16_t alternate_server_port = 0;
        std::initializer_list<uint16_t> unknown_attributes;
    };


    std::shared_ptr<stun_packet> from_buffer(const char *data, size_t length);

    bool is_maybe_stun(const char *data, size_t length);

    void stun_add_attribute(const std::shared_ptr<buffer> &buf, attribute_type &attr);
};// namespace stun

#endif//TOOLKIT_STUN_PACKET_H
