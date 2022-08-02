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
#include "stun_finger_print.h"

#include <Util/endian.hpp>
#include <Util/random.hpp>

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
            length += l - n;
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
        set_be16((void *) (buf->data() + 2), length);
        /// padding the length
        nearest_padding(buf, 4);
    }

    static void put_software(const std::string &software, const std::shared_ptr<buffer> &buf) {
        attribute_type attr;
        attr.attribute = attributes::software;
        attr.length = software.size();
        attr.value = software;
        stun_add_attribute(buf, attr);
    }

    static void put_realm(const std::string &realm, const std::shared_ptr<buffer> &buf) {
        attribute_type attr;
        attr.attribute = attributes::realm;
        attr.length = realm.size();
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
        attr.length = username.size();
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
        buf->put_be(static_cast<uint32_t>(stun_pkt.magic_cookie));
        /// transaction id
        std::string random_ = std::move(makeRandStr(12, true));
        buf->append(random_);
        /// software
        if (!stun_pkt.software.empty()) {
            put_software(stun_pkt.software, buf);
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

    void stun_packet::set_message_integrity(bool on) {
        this->message_integrity = on;
    }

    stun_packet from_buffer(const char *data, size_t length) {
        if (!is_stun(data, length)) {
        }
        return {};
    }

    bool is_stun(const char *data, size_t length) {

        return true;
    }

};// namespace stun