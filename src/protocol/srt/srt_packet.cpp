/*
* @file_name: srt_packet.cpp
* @date: 2022/04/27
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
#include "srt_packet.hpp"
#include "Util/endian.hpp"
namespace srt{

    bool srt_packet::is_control_packet() const{
        return data()[0] & 0x80;
    }

    srt_packet::srt_packet(){}

    srt_packet::srt_packet(basic_buffer<char>&& buf):basic_buffer<char>(std::move(buf)){

    }

    std::shared_ptr<srt_packet> srt_packet_helper::make_srt_control_packet(control_type type, uint32_t timestamp, uint32_t socket_id, uint32_t type_info){
        auto srt_buf = std::make_shared<srt_packet>();
        unsigned short control_type = (unsigned short)type | 0x8000;
        srt_buf->put_be(control_type);
        srt_buf->put_be(static_cast<unsigned short>(0));
        srt_buf->put_be(type_info);
        srt_buf->put_be(timestamp);
        srt_buf->put_be(socket_id);
        return srt_buf;
    }

    std::shared_ptr<srt_packet> srt_packet_helper::make_srt_data_packet(){
        return nullptr;
    }
};