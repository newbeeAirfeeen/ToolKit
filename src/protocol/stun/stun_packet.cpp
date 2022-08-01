/*
* @file_name: stun_packet.cpp
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

#include "stun_packet.h"
#include "Util/crc32.hpp"
#include <Util/endian.hpp>
namespace stun {
    /// Since all STUN attributes are
    /// padded to a multiple of 4 bytes, the last 2 bits of this field are
    /// always zero.
    static size_t nearest_padding(const std::shared_ptr<buffer> &buf, size_t padding) {
        size_t n = buf->size();
        /// ensure the length of buf
        if (buf->size() < padding) {
            buf->clear();
            buf->append(static_cast<size_t>(padding), static_cast<char>(0));
            return padding;
        }
        /// get the nearest padding value of n
        auto l = padding * (n / padding);
        if (l < n) {
            buf->append(static_cast<size_t>(n - l), static_cast<char>(0));
            auto length = load_be16(buf->data() + 2);
            length += n - l;
            set_be16((void *) (buf->data() + 2), static_cast<uint16_t>(length));
        }
        return n - l;
    }

    static void stun_add_attribute(const std::shared_ptr<buffer>& buf, attribute_type& attr){
        if(buf->size() < 4){
            throw std::bad_function_call();
        }
        /// TLV
        std::string bytes = std::move(attr.to_bytes());
        buf->append(bytes);
        auto length = load_be16(buf->data() + 2) + bytes.size();
        /// update the new length
        set_be16((void *)(buf->data() + 2), length);
        /// padding the length
        nearest_padding(buf, 4);
    }

    }



    std::shared_ptr<buffer> stun_packet::create_stun_packet(const stun_packet& stun_pkt){
        auto buf = std::make_shared<buffer>();



        if(stun_pkt.finger_print){
            finger_print(buf);
        }
        return buf;
    }


    stun_packet::stun_packet(const stun_method& m){
        this->_method = m;
    }

    void stun_packet::set_finger_print(bool on){
        this->finger_print = on;
    }



    stun_packet from_buffer(const char* data, size_t length){
        if(!is_stun(data, length)){

        }
    }

    bool is_stun(const char* data, size_t length){

    }

};