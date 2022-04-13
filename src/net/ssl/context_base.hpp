/*
* @file_name: context_base.hpp
* @date: 2022/04/12
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
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef TOOLKIT_CONTEXT_BASE_HPP
#define TOOLKIT_CONTEXT_BASE_HPP
#if defined(SSL_ENABLE) && defined(USE_OPENSSL)
#include <openssl/ssl.h>
class context_base{
public:
    struct tls {
        enum method {
            /*!
             * Generic SSL version 2
             */
            sslv2,
            /*!
             * SSL version 2 client
             */
            sslv2_client,
            /*!
             * SSL version 2 server
             */
            sslv2_server,
            /*!
             * Generic SSL version 3
             */
            sslv3,
            /*!
             * SSL version 3 client
             */
            sslv3_client,
            /*!
             * SSL version 3 server
             */
            sslv3_server,
            /*!
             * Generic TLS version 1
             */
            sslv1,
            /*!
             * tls version 1 client
             */
            sslv1_client,
            /*!
             * tls version 1 server
             */
            sslv1_server,
            /*!
             * tls version 1.1
             */
            sslv1_1,
            /*!
             * tls version 1.1 client
             */
            sslv1_1_client,
            /*!
             * tls version 1.1 server
             */
            sslv1_1_server,
            /*!
             * Generic SSL/TLS version 2,version 3
             */
            sslv23,
            /*!
             * ssl client with version 2, version 3
             */
            sslv23_client,
            /*!
             * ssl server with version 2, version 3
             */
            sslv23_server,
            /*!
             * Generic SSL/TLS version 1, version 2
             */
            sslv12,
            /*!
             * TLS/SSL client with version 1, version 2
             */
            sslv12_client,
            /*!
             * TLS/SSL server with version 1, version 2
             */
            sslv12_server,
            /*!
             * Generic SSL/TLS version 1, version 3
             */
            sslv13,
            /*!
             * SSL/TLS client with version 1, version 3
             */
            sslv13_client,
            /*!
             * SSL/TLS server with version 1, version 3
             */
            sslv13_server,
        };
    };

    struct dtls {
        enum method {

        };
    };

    /*!
     * File format types.
     */
    enum file_format
    {
       /*!
        * File format types.
        */
        asn1,
       /*!
        * PEM file.
        */
        pem,
    };


    /// Purpose of PEM password.
    enum password_purpose
    {
        /// The password is needed for reading/decryption.
        for_reading,

        /// The password is needed for writing/encryption.
        for_writing
    };

    /// client or server verify mode
    enum verify_mode {
        verify_none = SSL_VERIFY_NONE,
        verify_peer = SSL_VERIFY_PEER,
        verify_fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        verify_client_once = SSL_VERIFY_CLIENT_ONCE,
    };

public:
    using options = long;
#define OPTION_TYPE(name, value) static constexpr const long name = value;
    OPTION_TYPE(default_workarounds, SSL_OP_ALL);
    OPTION_TYPE(single_dh_use, SSL_OP_SINGLE_DH_USE);
    OPTION_TYPE(no_sslv2, SSL_OP_NO_SSLv2);
    OPTION_TYPE(no_sslv3, SSL_OP_NO_SSLv3);
    OPTION_TYPE(no_sslv1, SSL_OP_NO_TLSv1);
#ifdef SSL_OP_NO_TLSv1_1
    OPTION_TYPE(no_tlsv1_1, SSL_OP_NO_TLSv1_1);
#else
    OPTION_TYPE(no_tlsv1_1, 0x10000000U);
#endif
#ifdef SSL_OP_NO_TLSv1_2
    OPTION_TYPE(no_tlsv1_2, SSL_OP_NO_TLSv1_2);
#else
    OPTION_TYPE(no_tlsv1_2, 0x08000000U);
#endif
#ifdef SSL_OP_NO_TLSv1_3
    OPTION_TYPE(no_tlsv1_3, SSL_OP_NO_TLSv1_3);
#else
    OPTION_TYPE(no_tlsv1_3, 0x20000000U);
#endif // defined(SSL_OP_NO_TLSv1_3)
#ifdef SSL_OP_NO_COMPRESSION
    OPTION_TYPE(no_compression, SSL_OP_NO_COMPRESSION);
#else
    OPTION_TYPE(no_compression, 0x00020000U);
#endif
protected:
    ~context_base() = default;
};
#endif
#endif//TOOLKIT_CONTEXT_BASE_HPP
