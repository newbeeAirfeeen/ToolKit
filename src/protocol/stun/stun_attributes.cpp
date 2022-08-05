/*
* @file_name: stun_attributes.cpp
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

#include "stun_attributes.h"
#include "Util/endian.hpp"
#include "stun_packet.h"

#include <map>
namespace stun {


    std::string attribute_type::to_bytes() {
        std::string bytes;
        bytes.reserve(4 + value.size());
        uint16_t attr = htons(static_cast<uint16_t>(attribute));
        uint16_t len = htons(length);
        bytes.append((const char *) &attr, sizeof(uint16_t));
        bytes.append((const char *) &len, sizeof(len));
        bytes.append(value.data(), length);
        return std::move(bytes);
    }

    bool is_unknown_attributes(uint16_t type) {
        switch (type) {
            case mapped_address:
            case username:
            case message_integrity:
            case error_code:
            case unknown_attributes:
            case realm:
            case nonce:
            case xor_mapped_address:
            case software:
            case alternate_server:
            case finger_print:
                return false;
        }
        return true;
    }

    void put_unknown_attributes(const std::shared_ptr<buffer> &buf, const std::vector<uint16_t> &attrs) {
        buffer b;
        for (auto i: attrs) {
            b.put_be<uint16_t>(i);
        }
        attribute_type attr;
        attr.attribute = attributes::unknown_attributes;
        attr.value.assign(b.begin(), b.end());
        attr.length = static_cast<uint16_t>(attr.value.size());
        stun_add_attribute(buf, attr);
    }


    const char *get_attribute_name(const attributes &attr) {
        static const char *empty_string = "";
        static const std::map<uint16_t, const char *> attributes_map = {
                {attributes::mapped_address, "mapped_address"},
                {attributes::username, "username"},
                {attributes::message_integrity, "message_integrity"},
                {attributes::error_code, "error_code"},
                {attributes::unknown_attributes, "unknown_attributes"},
                {attributes::realm, "realm"},
                {attributes::nonce, "nonce"},
                {attributes::xor_mapped_address, "xor_mapped_address"},
                {attributes::software, "software"},
                {attributes::alternate_server, "alternate_server"},
                {attributes::finger_print, "finger_print"},
        };
        auto it = attributes_map.find(attr);
        if (it == attributes_map.end()) {
            return empty_string;
        }
        return it->second;
    }
};// namespace stun