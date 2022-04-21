#if 0
/*
* @file_name: context.cpp
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
#include "context.hpp"
#include <Util/onceToken.h>
#include <mutex>
#if defined(SSL_ENABLE) && defined(USE_OPENSSL)
#include "spdlog/logger.hpp"
#include <openssl/err.h>
std::string get_error_string(unsigned long err) {
    char buf[4096] = {0};
    ERR_error_string_n(err, buf, sizeof(buf));
    return {buf, std::char_traits<char>::length(buf)};
}


context::context(tls_method m) : handle_(nullptr) {
    init();
    switch (m) {
        //SSL v2
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) || defined(OPENSSL_NO_SSL2)
        case context::tls::method::sslv2:
        case context::tls::method::sslv2_client:
        case context::tls::method::sslv2_server:
            throw std::invalid_argument("ssl is no ssl2");
#else
        case context::tls::method::sslv2:
            handle_ = ::SSL_CTX_new(::SSLv2_method());
            break;
        case context::tls::method::sslv2_client:
            handle_ = ::SSL_CTX_new(::SSLv2_client_method());
            break;
        case context::sslv2_server:
            handle_ = ::SSL_CTX_new(::SSLv2_server_method());
            break;
#endif
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv3:
            handle_ = ::SSL_CTX_new(::TLS_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, SSL3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, SSL3_VERSION);
            }
            break;
        case context::tls::method::sslv3_client:
            handle_ = ::SSL_CTX_new(::TLS_client_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, SSL3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, SSL3_VERSION);
            }
            break;
        case context::tls::method::sslv3_server:
            handle_ = ::SSL_CTX_new(::TLS_server_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, SSL3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, SSL3_VERSION);
            }
            break;
#elif defined(OPENSSL_NO_SSL3)
        case context::tls::method::sslv3:
        case context::tls::method::sslv3_client:
        case context::tls::method::sslv3_server:
            throw std::invalid_argument("ssl not have ssl3");
#else
        case context::tls::method::sslv3:
            handle_ = ::SSL_CTX_new(::SSLv3_method());
            break;
        case context::tls::method::sslv3_client:
            handle_ = ::SSL_CTX_new(::SSLv3_client_method());
            break;
        case context::tls::method::sslv3_server:
            handle_ = ::SSL_CTX_new(::SSLv3_server_method());
            break;
#endif
            //TLS version 1
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv1:
            handle_ = ::SSL_CTX_new(::TLS_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_VERSION);
            }
            break;
        case context::tls::method::sslv1_client:
            handle_ = ::SSL_CTX_new(::TLS_client_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_VERSION);
            }
            break;
        case context::tls::method::sslv1_server:
            handle_ = ::SSL_CTX_new(::TLS_server_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_VERSION);
            }
            break;
#elif defined(SSL_TXT_TLSV1)
        case context::tls::method::sslv1:
            handle_ = ::SSL_CTX_new(::TLSv1_method());
            break;
        case context::tls::method::sslv1_client:
            handle_ = ::SSL_CTX_new(::TLSv1_client_method());
            break;
        case context::tls::method::sslv1_server:
            handle_ = ::SSL_CTX_new(::TLSv1_server_method());
            break;
#else
        case context::tls::method::sslv1:
        case context::tls::method::sslv1_client:
        case context::tls::method::sslv1_server:
            throw std::invalid_argument("ssl no sslv1, sslv1_client or sslv1_server method");
            break;
#endif
            // TLS v1.1.
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv1_1:
            handle_ = ::SSL_CTX_new(::TLS_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_1_VERSION);
            }
            break;
        case context::tls::method::sslv1_1_client:
            handle_ = ::SSL_CTX_new(::TLS_client_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_1_VERSION);
            }
            break;
        case context::tls::method::sslv1_1_server:
            handle_ = ::SSL_CTX_new(::TLS_server_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_1_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_1_VERSION);
            }
            break;
#elif defined(SSL_TXT_TLSV1_1)
        case context::tls::method::sslv1_1:
            handle_ = ::SSL_CTX_new(::TLSv1_1_method());
            break;
        case context::tls::method::sslv1_1_client:
            handle_ = ::SSL_CTX_new(::TLSv1_1_client_method());
            break;
        case context::tls::method::sslv1_1_server:
            handle_ = ::SSL_CTX_new(::TLSv1_1_server_method());
            break;
#else // defined(SSL_TXT_TLSV1_1)
        case context::tls::method::sslv1_1:
        case context::tls::method::sslv1_1_client:
        case context::tls::method::sslv1_1_server:
            throw std::invalid_argument("ssl not have sslv1_1");
            break;
#endif// defined(SSL_TXT_TLSV1_1)

            // TLS v1.2.
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv12:
            handle_ = ::SSL_CTX_new(::TLS_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_2_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_2_VERSION);
            }
            break;
        case context::tls::method::sslv12_client:
            handle_ = ::SSL_CTX_new(::TLS_client_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_2_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_2_VERSION);
            }
            break;
        case context::tls::method::sslv12_server:
            handle_ = ::SSL_CTX_new(::TLS_server_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_2_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_2_VERSION);
            }
            break;
#elif defined(SSL_TXT_TLSV1_2)
        case context::tls::method::sslv12:
            handle_ = ::SSL_CTX_new(::TLSv1_2_method());
            break;
        case context::tls::method::sslv12_client:
            handle_ = ::SSL_CTX_new(::TLSv1_2_client_method());
            break;
        case context::tls::method::sslv12_server:
            handle_ = ::SSL_CTX_new(::TLSv1_2_server_method());
            break;
#else // defined(SSL_TXT_TLSV1_2)
        case context::tls::method::sslv12:
        case context::tls::method::sslv12_client:
        case context::tls::method::sslv12_server:
            throw std::invalid_argument("ssl not have ssl version 2");
#endif// defined(SSL_TXT_TLSV1_2)

            // TLS v1.3.
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L) && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv13:
            handle_ = ::SSL_CTX_new(::TLS_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_3_VERSION);
            }
            break;
        case context::tls::method::sslv13_client:
            handle_ = ::SSL_CTX_new(::TLS_client_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_3_VERSION);
            }
            break;
        case context::tls::method::sslv13_server:
            handle_ = ::SSL_CTX_new(::TLS_server_method());
            if (handle_) {
                SSL_CTX_set_min_proto_version(handle_, TLS1_3_VERSION);
                SSL_CTX_set_max_proto_version(handle_, TLS1_3_VERSION);
            }
            break;
#else // (OPENSSL_VERSION_NUMBER >= 0x10101000L) \
        //   && !defined(LIBRESSL_VERSION_NUMBER)
        case context::tls::method::sslv13:
        case context::tls::method::sslv13_client:
        case context::tls::method::sslv13_server:
            throw std::invalid_argument("ssl not have ssl version 1, and 3");
            break;
#endif// (OPENSSL_VERSION_NUMBER >= 0x10101000L) \
        //   && !defined(LIBRESSL_VERSION_NUMBER)

            // Any supported SSL/TLS version.
        case context::tls::method::sslv23:
            handle_ = ::SSL_CTX_new(::SSLv23_method());
            break;
        case context::tls::method::sslv23_client:
            handle_ = ::SSL_CTX_new(::SSLv23_client_method());
            break;
        case context::tls::method::sslv23_server:
            handle_ = ::SSL_CTX_new(::SSLv23_server_method());
            break;
        default:
            handle_ = ::SSL_CTX_new(0);
            break;
    }

    if (handle_ == 0) {
        throw std::invalid_argument(get_error_string());
    }
    //set_options(no_compression);
}

context::context(dtls_method m) {
    init();
#if 0
    switch(m){
#ifdef OPENSSL_NO_DTLS1_METHOD
        case context_base::dtls::sslv1:
        case context_base::dtls::sslv1_server:
        case context_base::dtls::sslv1_client:
            throw std::invalid_argument("dtls setting error: unknown dtls method");
#else
        case context_base::dtls::sslv1:
            handle_ = ::SSL_CTX_new(DTLSv1_method());
            break;
        case context_base::dtls::sslv1_server:
            handle_ = ::SSL_CTX_new(DTLSv1_server_method());
            break;
        case context_base::dtls::sslv1_client:
            handle_ = ::SSL_CTX_new(DTLSv1_client_method());
            break;
#endif
#ifdef OPENSSL_NO_DTLS1_2_METHOD
        case context_base::dtls::sslv1_2:
        case context_base::dtls::sslv1_2_server:
        case context_base::dtls::sslv1_2_client:
            throw std::invalid_argument("dtls setting error: no dtls v1.2 method");
#else
        case context_base::dtls::sslv1_2:
            handle_ = ::SSL_CTX_new(DTLSv1_2_method());
            break;
        case context_base::dtls::sslv1_2_server:
            handle_ = ::SSL_CTX_new(DTLSv1_2_server_method());
            break;
        case context_base::dtls::sslv1_2_client:
            handle_ = ::SSL_CTX_new(DTLSv1_2_client_method());
            break;
#endif
        default:
            throw std::invalid_argument("dtls setting error: unknown dtls method");
    }
#else
    switch (m) {
        case context_base::dtls::method::ssl_client:
            handle_ = ::SSL_CTX_new(DTLS_client_method());
            break;
        case context_base::dtls::method::ssl_server:
            handle_ = ::SSL_CTX_new(DTLS_server_method());
            break;
        default:
            throw std::invalid_argument("no dtls method");
    }
#endif
    if (handle_ == nullptr) {
        throw std::invalid_argument("dtls initialize error: not init a method");
    }
}

context::context(context &&other) noexcept {
    handle_ = other.handle_;
    other.handle_ = nullptr;
}

context &context::operator=(context &&other) {
    context tmp(std::move(static_cast<context &>(*this)));
    std::swap(handle_, other.handle_);
    return *this;
}

context::~context() {
    if (!handle_) return;
    ::SSL_CTX_free(handle_);
}

using native_handle_type = typename context::native_handle_type;
native_handle_type context::native_handle() {
    return handle_;
}

void context::clear_options(options o) {
#if (OPENSSL_VERSION_NUMBER >= 0x009080DFL) && (OPENSSL_VERSION_NUMBER != 0x00909000L)
#if !defined(SSL_OP_NO_COMPRESSION)
    if ((o & context::no_compression) != 0) {
#if (OPENSSL_VERSION_NUMBER >= 0x00908000L)
        handle_->comp_methods = SSL_COMP_get_compression_methods();
#endif// (OPENSSL_VERSION_NUMBER >= 0x00908000L)
        o ^= context::no_compression;
    }
#endif// !defined(SSL_OP_NO_COMPRESSION)
    Error(get_error_string(::SSL_CTX_clear_options(handle_, o)));
#endif// (OPENSSL_VERSION_NUMBER >= 0x009080DFL)
}

void context::set_options(options o) {
#if !defined(SSL_OP_NO_COMPRESSION)
    if ((o & context::no_compression) != 0) {
        o ^= context::no_compression;
    }
#endif// !defined(SSL_OP_NO_COMPRESSION)
    Error(get_error_string(::SSL_CTX_clear_options(handle_, o)));
}


void context::set_verify_mode(verify_mode v) {
    ::SSL_CTX_set_verify(handle_, v, nullptr);
}


void context::load_verify_file(const std::string &filename) {
    if (::SSL_CTX_load_verify_locations(handle_, filename.c_str(), 0) != 1) {
        throw std::runtime_error(get_error_string());
    }
}

void context::set_default_verify_paths() {
    if (::SSL_CTX_set_default_verify_paths(handle_) != 1) {
        throw std::runtime_error(get_error_string());
    }
}

void context::use_certificate_chain_file(const std::string &filename) {
    ::ERR_clear_error();
    if (::SSL_CTX_use_certificate_chain_file(handle_, filename.c_str()) != 1) {
        throw std::runtime_error(get_error_string());
    }
}

void context::use_private_key_file(const std::string &filename, file_format format) {
    if (::SSL_CTX_use_PrivateKey_file(handle_, filename.c_str(),
                                      format == file_format::pem ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1) != 1) {

        throw std::runtime_error(get_error_string());
    }

    auto ret = ::SSL_CTX_check_private_key(handle_);
    if (ret != 1) {
        throw std::runtime_error(get_error_string());
    }
}


void context::init() {
    static toolkit::onceToken token([&]() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        OpenSSL_add_all_digests();
        CRYPTO_set_locking_callback([](int mode, int n, const char *file, int line) {
            static mutex *s_mutexes = new mutex[CRYPTO_num_locks()];
            static onceToken token(nullptr, []() {
                delete[] s_mutexes;
            });
            if (mode & CRYPTO_LOCK) {
                s_mutexes[n].lock();
            } else {
                s_mutexes[n].unlock();
            }
        });

        CRYPTO_set_id_callback([]() -> unsigned long {
            return (unsigned long) std::this_thread::get_id();
        });
#endif
    });
}
#endif
#endif