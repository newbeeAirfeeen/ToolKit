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
#include "stun_packet.h"
static size_t HMAC_SHA1(const char *key, size_t length, const char *data, size_t data_length, void *out, size_t out_length) {
    HMAC_CTX *ctx = new HMAC_CTX;
    HMAC_CTX_init(ctx);
    HMAC_Init_ex(ctx, (const void *) key, length, EVP_sha1(), nullptr);
    HMAC_Update(ctx, (const unsigned char *) data, strlen(data));
    auto ret = HMAC_Final(ctx, (unsigned char *) out, (unsigned int *) &out_length);
    HMAC_CTX_cleanup(ctx);
    if (ctx) delete ctx;
}

namespace stun {
    void put_message_integrity(const stun_packet &pkt, const std::shared_ptr<buffer> &buf) {
    }
};// namespace stun
#endif
