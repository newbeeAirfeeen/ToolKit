/*
* @file_name: srt_handshake.cpp
* @date: 2022/04/29
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
#include "srt_handshake.hpp"
#include "srt_packet_profile.hpp"
#include "srt_packet.hpp"
#include "Util/endian.hpp"
namespace srt{

    bool handshake_packet::load(const control_packet_context& ctx, const srt_packet& pkt){
        if(!pkt.is_control_packet()){
            SRT_ERROR_LOG("not control packet");
            return false;
        }

        if(ctx.get_control_type() != srt::control_type::handshake){
            SRT_ERROR_LOG("the packet is not handshake control type");
            return false;
        }

        if(ctx.size() < static_cast<size_t>(handshake_packet::packet_size)){
            SRT_ERROR_LOG("the handshake packet size must greater than 48");
            return false;
        }
        const auto* pointer = (const uint32_t*)ctx.data();
        _version = load_be32(pointer++);
        if(_version != 4 && _version != 5){
            SRT_ERROR_LOG("the handshake packet is not 4 or 5");
            return false;
        }
        encryption = load_be16(pointer++);
        _sequence_number = load_be32(pointer++);
        _max_mss = load_be32(pointer++);
        if( _max_mss > 1500 ){
            SRT_ERROR_LOG("maximum mss is greater than 1500");
            return false;
        }
        _window_size = load_be32(pointer++);
        _req_type = (handshake_packet::packet_type)load_be32(pointer++);
        if(((uint32_t)_req_type) < 0xFFFFFFFD && ((uint32_t)_req_type) > 0x00000001){
            SRT_ERROR_LOG("unknown request type");
            return false;
        }
        _socket_id = load_be32(pointer++);
        _cookie = load_be32(pointer++);
        memcpy(_peer_ip, pointer, 16);
        return true;
    }

};