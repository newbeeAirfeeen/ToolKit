/*
* @file_name: stun_error.h
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

#ifndef TOOLKIT_STUN_ERROR_H
#define TOOLKIT_STUN_ERROR_H
#include <net/buffer.hpp>
#include <system_error>
/// the error code attribute is used in error response messages.
namespace stun {

    /// The ERROR-CODE attribute is used in error response messages. It
    /// contains a numeric error code value in the range of 300 to 699 plus a
    /// textual reason phrase encoded in UTF-8 [RFC3629], and is consistent
    /// in its code assignments and semantics with SIP [RFC3261] and HTTP
    /// [RFC2616].
    void put_error_code(uint32_t err, const std::shared_ptr<buffer>& buff);


    class stun_packet_category : public std::error_category {
    public:
        const char *name() const noexcept override;
        std::string message(int err) const override;
    };

    class category : public std::error_category {
    public:
        const char *name() const noexcept override;
        std::string message(int err) const override;
    };

    std::error_category *generate_stun_packet_category();
    std::error_category *generate_category();
    std::error_code make_stun_error(int err, const std::error_category *_category);
};// namespace stun

#endif//TOOLKIT_STUN_ERROR_H
