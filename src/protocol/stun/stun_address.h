/*
* @file_name: stun_address.h
* @date: 2022/08/03
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

#ifndef TOOLKIT_STUN_ADDRESS_H
#define TOOLKIT_STUN_ADDRESS_H
#include <memory>
#include <net/buffer.hpp>
#include "stun_attributes.h"
namespace stun {
    class stun_packet;
    void put_mapped_address_or_alternate_server(const attributes& a, const std::shared_ptr<buffer> &buf, const std::string &ip, uint16_t port);
    void put_xor_mapped_address(const std::shared_ptr<buffer> &buf, const std::string& transaction_id, const std::string &ip, uint16_t port);

    void get_mapped_address(const std::shared_ptr<stun_packet>& pkt, const char* data, size_t length);
};    // namespace stun
#endif//TOOLKIT_STUN_ADDRESS_H
