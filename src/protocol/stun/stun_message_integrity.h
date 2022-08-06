﻿/*
* @file_name: stun_message_integrity.h
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

#ifndef TOOLKIT_STUN_MESSAGE_INTEGRITY_HPP
#define TOOLKIT_STUN_MESSAGE_INTEGRITY_HPP
#ifdef WOLFSSL_ENABLE
#include <wolfssl/openssl/evp.h>
#include <wolfssl/openssl/hmac.h>
#elif OPENSSL_ENABLE
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

#ifdef SSL_ENABLE
#include <cstdint>
#include <memory>
#include <net/buffer.hpp>
namespace stun {
    class stun_packet;
    void put_message_integrity(const stun_packet &pkt, const std::shared_ptr<buffer> &buf, const std::string &username, const std::string &realm, const std::string &password);
};// namespace stun
#endif
#endif//TOOLKIT_STUN_MESSAGE_INTEGRITY_HPP