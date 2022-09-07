/*
* @file_name: stun_packet.cpp
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

#include "stun_packet.h"

#ifdef SSL_ENABLE
#include "stun_message_integrity.h"
#endif

#include "SASL_prep.h"
#include "Util/string_view.hpp"
#include "stun_address.h"
#include "stun_error.h"
#include "stun_error_code.h"
#include "stun_finger_print.h"
#include <Util/endian.hpp>
#include <Util/random.hpp>
#include <functional>
namespace stun {
    /// Since all STUN attributes are
    /// padded to a multiple of 4 bytes, the last 2 bits of this field are
    /// always zero.
    static size_t nearest_padding(const std::shared_ptr<buffer> &buf, size_t padding) {
        size_t n = buf->size();
        /// ensure the length of buf
        if (buf->size() < padding) {
            buf->clear();
            buf->append(static_cast<size_t>(padding), static_cast<char>(0));
            return padding;
        }
        /// get the nearest padding value of n
        auto l = padding * (n / padding) + padding;
        if (n % padding != 0) {
            buf->append(static_cast<size_t>(l - n), static_cast<char>(0));
            auto length = load_be16(buf->data() + 2);
            length +=  static_cast<uint16_t>(l - n);
            set_be16((void *) (buf->data() + 2), static_cast<uint16_t>(length));
        }
        return l - n;
    }

    void stun_add_attribute(const std::shared_ptr<buffer> &buf, attribute_type &attr) {
        if (buf->size() < 4) {
            throw std::bad_function_call();
        }
        /// TLV
        std::string bytes = std::move(attr.to_bytes());
        buf->append(bytes);
        auto length = load_be16(buf->data() + 2) + bytes.size();
        /// update the new length
        set_be16((void *) (buf->data() + 2),  static_cast<uint16_t>(length));
        /// padding the length
        nearest_padding(buf, 4);
    }

    static void put_software(const std::string &software, const std::shared_ptr<buffer> &buf) {
        attribute_type attr;
        attr.attribute = attributes::software;
        attr.length =  static_cast<uint16_t>(software.size());
        attr.value = software;
        stun_add_attribute(buf, attr);
    }

    static void put_realm(const std::string &realm, const std::shared_ptr<buffer> &buf) {
        attribute_type attr;
        attr.attribute = attributes::realm;
        attr.length = static_cast<uint16_t>(realm.size());
        attr.value = realm;
        stun_add_attribute(buf, attr);
    }


    static void put_username(const std::string &username, const std::shared_ptr<buffer> &buf) {
        /// It MUST contain a UTF-8 [RFC3629] encoded sequence of less than 513 bytes, and MUST have been processed using SASLprep
        if (username.size() > 513) {
            throw std::invalid_argument("the username length MUST less than 513");
        }

        auto ret = SASL_prep((uint8_t *) username.data());
        if (ret != 0) {
            throw std::invalid_argument("the username' value have invalid character");
        }

        attribute_type attr;
        attr.attribute = attributes::username;
        attr.length = static_cast<uint16_t>(username.size());
        attr.value = username;
        stun_add_attribute(buf, attr);
    }

    std::shared_ptr<buffer> stun_packet::create_packet(const stun_packet &stun_pkt) {
        auto buf = std::make_shared<buffer>();
        buf->reserve(128);
        /// put method into buffer
        uint16_t m = static_cast<uint16_t>(stun_pkt._method);
        buf->put_be(static_cast<uint16_t>(m));
        /// put message length
        buf->append(static_cast<size_t>(2), 0);
        /// put magic cookie
        buf->put_be(static_cast<uint32_t>(stun_packet::magic_cookie));
        /// transaction id
        std::string random_ = std::move(makeRandStr(12, true));
        buf->append(random_);

        if (!stun_pkt.mapped_address.empty()) {
            /// mapped-address
            put_mapped_address_or_alternate_server(attributes::mapped_address, buf, stun_pkt.mapped_address, stun_pkt.mapped_address_port);
        } else if (!stun_pkt.xor_mapped_address.empty()) {
            /// xor_mapped_address
            put_xor_mapped_address(buf, random_, stun_pkt.xor_mapped_address, stun_pkt.mapped_address_port);
        } else if (!stun_pkt.alternate_server.empty()) {
            /// alternate-server
            put_mapped_address_or_alternate_server(attributes::alternate_server, buf, stun_pkt.alternate_server, stun_pkt.alternate_server_port);
        }


        /// software
        if (!stun_pkt.software.empty()) {
            put_software(stun_pkt.software, buf);
        }

        /// unknown attribute
        if (stun_pkt.unknown_attributes.size()) {
            put_unknown_attributes(buf, stun_pkt.unknown_attributes);
        }

#ifdef SSL_ENABLE
        /// username - nonce - message-integrity
        if (stun_pkt.message_integrity) {
            put_username(stun_pkt.username, buf);
            put_realm(stun_pkt.realm, buf);
            put_message_integrity(stun_pkt, buf, stun_pkt.username, stun_pkt.realm, stun_pkt.password);
        }
#endif
        /// the FINGERPRINT attribute MUST be the last attribute in
        /// the message, and thus will appear after MESSAGE-INTEGRITY
        if (stun_pkt.finger_print) {
            put_finger_print(stun_pkt, buf);
        }
        return buf;
    }

    void stun_packet::set_method(const stun_method &m) {
        this->_method = m;
    }

    void stun_packet::set_finger_print(bool on) {
        this->finger_print = on;
    }

    void stun_packet::set_username(const std::string &username) {
        this->username = username;
    }

    void stun_packet::set_password(const std::string &password) {
        this->password = password;
    }

    void stun_packet::set_realm(const std::string &realm) {
        if (realm.size() > 128) {
            throw std::invalid_argument("realm MUST be a UTF-8 encoded sequence of less than 128");
        }
        this->realm = realm;
    }

    void stun_packet::set_software(const std::string &software) {
        if (software.size() > 128) {
            throw std::invalid_argument("software MUST be a UTF-8 encoded sequence of less than 128");
        }
        this->software = software;
    }

    void stun_packet::set_unknown_attributes(const std::vector<uint16_t> &attrs) {
        this->unknown_attributes = attrs;
    }

    void stun_packet::set_alternate_server(const std::string &ip, uint16_t port) {
        this->alternate_server = ip;
        this->alternate_server_port = port;
    }

    void stun_packet::set_mapped_address(const std::string &ip, uint16_t port) {
        this->mapped_address = ip;
        this->mapped_address_port = port;
    }

    void stun_packet::set_xor_mapped_address(const std::string &ip, uint16_t port) {
        this->xor_mapped_address = ip;
        this->mapped_address_port = port;
    }

#ifdef WOLFSSL_ENABLE
    void stun_packet::set_message_integrity(bool on) {
        this->message_integrity = on;
    }
#endif
    stun_method stun_packet::get_method() const {
        return this->_method;
    }

    const std::string &stun_packet::get_transaction_id() const {
        return this->transaction_id;
    }

    const std::string &stun_packet::get_username() const {
        return this->username;
    }

    const std::string &stun_packet::get_realm() const {
        return this->realm;
    }

    const std::string &stun_packet::get_password() const {
        return this->password;
    }

    const std::string &stun_packet::get_software() const {
        return this->software;
    }

    const std::vector<uint16_t> &stun_packet::get_unknown_attributes() const {
        return unknown_attributes;
    }

    std::pair<const std::string &, uint16_t> stun_packet::get_alternate_server() const {
        std::pair<const std::string &, uint16_t> pair_ = std::make_pair(std::cref(this->alternate_server), this->alternate_server_port);
        return pair_;
    }

    std::pair<const std::string &, uint16_t> stun_packet::get_mapped_address() const {
        auto _pair = std::make_pair(std::cref(this->mapped_address), this->mapped_address_port);
        return _pair;
    }

    std::pair<const std::string &, uint16_t> stun_packet::get_xor_mapped_address() const {
        auto _pair = std::make_pair(std::cref(this->xor_mapped_address), this->mapped_address_port);
        return _pair;
    }

    std::shared_ptr<stun_packet> from_buffer(const char *data, size_t length) {
        if (!is_maybe_stun(data, length)) {
            throw std::system_error(make_stun_error(stun::is_not_stun_packet, generate_stun_packet_category()));
        }

        auto stun_pkt = std::make_shared<stun_packet>();
        auto m = load_be16((const void *) data);
        if (!is_stun_method(m)) {
            throw std::system_error(make_stun_error(stun::is_not_stun_packet, generate_stun_packet_category()));
        }
        auto pkt_length = load_be16((const void *) (data + 2));
        stun_pkt->_method = static_cast<stun_method>(m);
        stun_pkt->transaction_id.assign(data + 8, 12);
        auto *pointer = data + 20;
        auto *end = data + length;

        while (pointer != end && (end - pointer > 4)) {
            /// T
            auto type = load_be16(pointer);
            pointer += 2;
            /// L
            auto attr_length = load_be16(pointer);
            uint32_t padding = 0;
            /// if have padding
            if (attr_length % 4 != 0) {
                padding = attr_length / 4 * 4 + padding - attr_length;
            }
            pointer += 2;
            /// V
            switch (type) {
                case mapped_address:
                    break;
                case username:
                    break;
                case message_integrity:
                    break;
                case error_code:
                case unknown_attributes:
                case realm:
                case nonce:
                case xor_mapped_address:
                case software:
                case alternate_server:
                case finger_print:
                default: {
                    stun_pkt->unknown_attributes.push_back(type);
                }
            }
            pointer += attr_length + padding;
        }


        return nullptr;
    }

    bool is_maybe_stun(const char *data, size_t length) {
        /// check header length
        if (length < 20) {
            return false;
        }

        /// check magic cookie
        if (load_be32(data + 4) != stun_packet::magic_cookie) {
            return false;
        }

        return true;
    }

};// namespace stun