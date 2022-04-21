#if 0
/*
* @file_name: context.hpp
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
#ifndef TOOLKIT_CONTEXT_HPP
#define TOOLKIT_CONTEXT_HPP
#include "context_base.hpp"
#include <Util/nocopyable.hpp>
#include <system_error>
#if defined(SSL_ENABLE) && defined(USE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>
std::string get_error_string(unsigned long err = ::ERR_get_error());
class context : public context_base, public noncopyable {
public:
    using native_handle_type = SSL_CTX*;
    using tls_method = typename context_base::tls::method;
    using dtls_method = typename context_base::dtls::method;

public:
    explicit context(tls_method);
    explicit context(dtls_method);

    context(context &&other) noexcept ;
    context& operator=(context&& other);
    ~context();

    native_handle_type native_handle();
    /// Clear options on the context.
    /**
     * This function may be used to configure the SSL options used by the context.
     *
     * @param o A bitmask of options. The available option values are defined in
     * the context_base class. The specified options, if currently enabled on the
     * context, are cleared.
     *
     * @param ec Set to indicate what error occurred, if any.
     *
     * @note Calls @c SSL_CTX_clear_options.
     */
    void clear_options(options o);
    /// Set options on the context.
    /**
     * This function may be used to configure the SSL options used by the context.
     *
     * @param o A bitmask of options. The available option values are defined in
     * the context_base class. The options are bitwise-ored with any existing
     * value for the options.
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_set_options.
     */
    void set_options(options o);

    /// Set the peer verification mode.
    /**
     * This function may be used to configure the peer verification mode used by
     * the context.
     *
     * @param v A bitmask of peer verification modes. See @ref verify_mode for
     * available values.
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_set_verify.
     */
    void set_verify_mode(verify_mode v);

    /// Load a certification authority file for performing verification.
    /**
     * This function is used to load one or more trusted certification authorities
     * from a file.
     *
     * @param filename The name of a file containing certification authority
     * certificates in PEM format.
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_load_verify_locations.
     */
    void load_verify_file(const std::string &filename);
    /// Configures the context to use the default directories for finding
    /// certification authority certificates.
    /**
     * This function specifies that the context should use the default,
     * system-dependent directories for locating certification authority
     * certificates.
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_set_default_verify_paths.
     */
    void set_default_verify_paths();

    /// Use a certificate chain from a file.
    /**
     * This function is used to load a certificate chain into the context from a
     * file.
     *
     * @param filename The name of the file containing the certificate. The file
     * must use the PEM format.
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_use_certificate_chain_file.
     */
    void use_certificate_chain_file(const std::string& filename);
    /// Use a private key from a file.
    /**
     * This function is used to load a private key into the context from a file.
     *
     * @param filename The name of the file containing the private key.
     *
     * @param format The file format (ASN.1 or PEM).
     *
     * @throws std::runtime_error Thrown on failure.
     *
     * @note Calls @c SSL_CTX_use_PrivateKey_file.
     */
     void use_private_key_file(const std::string& filename, file_format format);
 private:
     void init();
private:
    native_handle_type handle_;
};
#endif

#endif//TOOLKIT_CONTEXT_HPP
#endif