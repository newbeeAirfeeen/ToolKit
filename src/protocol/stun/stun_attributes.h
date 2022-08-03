/*
* @file_name: stun_attributes.h
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

#ifndef TOOLKIT_STUN_ATTRIBUTES_H
#define TOOLKIT_STUN_ATTRIBUTES_H
#include <cstdint>
#include <string>
#include <net/buffer.hpp>
namespace stun {
    enum attributes {
        /// Comprehension-required range
        mapped_address = 0x0001,
        username = 0x0006,
        message_integrity = 0x0008,
        error_code = 0x0009,
        /// The UNKNOWN-ATTRIBUTES attribute is present only in an error response
        /// when the response code in the ERROR-CODE attribute is 420.
        /// The attribute contains a list of 16-bit values, each of which
        /// represents an attribute type that was not understood by the server
        unknown_attributes = 0x000A,
        /// The REALM attribute may be present in requests and responses. It
        /// contains text that meets the grammar for "realm-value" as described
        /// in RFC 3261 [RFC3261] but without the double quotes and their
        /// surrounding whitespace.
        realm = 0x0014,
        /// The NONCE attribute may be present in requests and response
        ///
        nonce = 0x0015,
        xor_mapped_address = 0x0020,
        /// Comprehension-optional range (0x8000-0xFFFF)
        software = 0x8022,
        /// The alternate server represents an alternate transport address
        /// identifying a different STUN server that the STUN client should try.
        /// It is encoded in the same way as MAPPED-ADDRESS, and thus refers to a
        /// single server by IP address. The IP address family MUST be identical
        /// to that of the source IP address of the request.
        alternate_server = 0x8023,
        /// The FINGERPRINT attribute MAY be present in all STUN messages
        /// The value of the attribute is computed as the CRC-32 of the STUN message
        /// up to (but excluding) the FINGERPRINT attribute itself
        ///  the FINGERPRINT attribute MUST be the last attribute in
        /// the message, and thus will appear after MESSAGE-INTEGRITY
        finger_print = 0x8028,
    };

    struct attribute_type {
        attributes attribute;
        uint16_t length;
        std::string value;
        std::string to_bytes();
    };

    void put_unknown_attributes(const std::shared_ptr<buffer>& buff, const std::initializer_list<uint16_t>& attrs);

    const char *get_attribute_name(const attributes &);
};// namespace stun


#endif//TOOLKIT_STUN_ATTRIBUTES_H
