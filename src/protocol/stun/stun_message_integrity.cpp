/*
* @file_name: stun_message_integrity.cpp
* @date: 2022/08/01
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
#ifdef SSL_ENABLE
#include "stun_message_integrity.h"
#include "SASL_prep.h"
#include "Util/MD5.h"
#include "Util/endian.hpp"
#include "stun_packet.h"

static size_t HMAC_SHA1(const char *key, size_t length, const char *data, size_t data_length, void *out, size_t out_length) {
    HMAC_CTX *ctx = new HMAC_CTX;
    HMAC_CTX_init(ctx);
    HMAC_Init_ex(ctx, (const void *) key, length, EVP_sha1(), nullptr);
    HMAC_Update(ctx, (const unsigned char *) data, strlen(data));
    auto ret = HMAC_Final(ctx, (unsigned char *) out, (unsigned int *) &out_length);
    HMAC_CTX_cleanup(ctx);
    if (ctx) delete ctx;
    return ret;
}

namespace stun {
    /// The MESSAGE-INTEGRITY attribute contains an HMAC-SHA1 [RFC2104] of
    /// the STUN message. The MESSAGE-INTEGRITY attribute can be present in
    /// any STUN message type. Since it uses the SHA1 hash, the HMAC will be
    /// 20 bytes. The text used as input to HMAC is the STUN message,
    /// including the header, up to and including the attribute preceding the
    /// MESSAGE-INTEGRITY attribute. With the exception of the FINGERPRINT
    /// attribute, which appears after MESSAGE-INTEGRITY, agents MUST ignore
    /// all other attributes that follow MESSAGE-INTEGRITY.
    /// For long-term credentials, the key is 16
    ///
    /// key = MD5(username ":" realm ":" SASLprep(password))
    void put_message_integrity(const stun_packet &pkt, const std::shared_ptr<buffer> &buf, const std::string &username,
                               const std::string &realm, const std::string &password) {
        constexpr int message_integrity_length = 20;
        constexpr int attribute_header_length = 4;
        if (buf->size() < 4) {
            throw std::bad_function_call();
        }

        /// get the origin length
        auto origin_length = load_be16(buf->data() + 2);
        /// write the new length
        auto new_length = origin_length + message_integrity_length + attribute_header_length;
        /// update the new length in message
        set_be16((void *) (buf->data() + 2), new_length);

        int ret = SASL_prep((uint8_t *) password.data());
        if (ret != 0) {
            throw std::invalid_argument("message_integrity: password is invalid.");
        }
        /// calculate the key
        std::string _ = username + ":" + realm + ":" + password;
        toolkit::MD5_digest digest(_);
        std::string key = digest.rawdigest();

        char sha1_digest[20] = {0};
        auto length = HMAC_SHA1(key.data(), key.size(), buf->data(),
                                buf->size(), sha1_digest, sizeof(sha1_digest));
        if (length != 1) {
            throw std::runtime_error("put_message_integrity HMAC_SHA1 digest error");
        }

        /// recover the origin length
        set_be16((void *) (buf->data() + 2), origin_length);


        attribute_type attr;
        attr.attribute = message_integrity;
        attr.length = 20;
        attr.value.assign(sha1_digest, 20);
        stun_add_attribute(buf, attr);
    }
};// namespace stun
#endif
