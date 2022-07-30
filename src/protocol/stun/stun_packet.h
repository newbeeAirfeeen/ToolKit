/*
* @file_name: stun_packet.h
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


#ifndef TOOLKIT_STUN_PACKET_H
#define TOOLKIT_STUN_PACKET_H
#include "stun_method.h"
#include "stun_attributes.h"
#include "net/buffer.hpp"
namespace stun{

    struct stun_packet{
        stun_method _method = binding_request;
        uint16_t message_length = 0;
        const uint32_t magic_cookie =  0x2112A442;
        char transaction[12] = {0};
    };

    std::shared_ptr<buffer> create_packet(const stun_method& m);

    stun_packet from_buffer(const char* data, size_t length);

    bool is_stun(const char* data, size_t length);
};

#endif//TOOLKIT_STUN_PACKET_H
