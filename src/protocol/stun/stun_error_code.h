/*
* @file_name: stun_error_code.h
* @date: 2022/08/02
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

#ifndef TOOLKIT_STUN_ERROR_CODE_HPP
#define TOOLKIT_STUN_ERROR_CODE_HPP

namespace stun {

    enum {
        is_not_stun_packet,        /// is not stun packet.
        error_code_is_invalid,     /// error code is not range of 300-699
        attribute_is_invalid,      /// attribute is invalid.
        address_family_is_invalid, /// stun mapped address family is invalid
    };

    enum stun_error_code {
        try_alternate = 300,
        bad_request = 400,
        unauthorized = 401,
        unknown_attribute = 420,
        stale_nonce = 438,
        server_error = 500,
    };
};// namespace stun


#endif//TOOLKIT_STUN_ERROR_CODE_HPP
