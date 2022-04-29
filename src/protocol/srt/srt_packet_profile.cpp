/*
* @file_name: srt_packet_profile.cpp
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
#include "srt_packet_profile.hpp"
#include "srt_packet.hpp"
#include "Util/endian.hpp"
#include "err.hpp"
#include <type_traits>
#include <new>
namespace srt{

    control_packet_context::control_packet_context(const srt_packet& pkt){
        const char* data = pkt.data();
        type = static_cast<control_type>(load_be16(data) & 0x7FFF);
        sub_type = load_be16(&(data[2]));
        type_information = load_be32(&(data[4]));
        time_stamp = load_be32(&(data[8]));
        socket_id = load_be32(&(data[12]));
        raw_data = pkt.data() + 16;
        raw_size = pkt.size() - 16;
    }

    control_type control_packet_context::get_control_type() const{
        return type;
    }

    unsigned short control_packet_context::get_sub_type() const{
        return sub_type;
    }

    uint32_t control_packet_context::get_type_information() const{
        return type_information;
    }

    uint32_t control_packet_context::get_timestamp() const{
        return time_stamp;
    }

    uint32_t control_packet_context::get_socket_id() const{
        return socket_id;
    }

    const char* control_packet_context::data() const{
        return raw_data;
    }

    size_t control_packet_context::size() const{
        return raw_size;
    }

    data_packet_context::data_packet_context(const srt_packet& pkt){
        const char* data = pkt.data();
        packet_sequence_number = static_cast<control_type>(load_be32(data) & 0x7FFFFFFF);
        packet_position = data[4] & 0xC0;
        should_order_ = (data[4] & 0x20) == 1;
        key_encrypt_ = data[4] & 0x18;
        is_retransmitted_ = data[4] & 0x4;
        message_number = load_be32(&(data[4])) & 0x3FFFFFF;
        time_stamp = load_be32(&(data[8]));
        socket_id = load_be32(&(data[12]));
        raw_data = data + 16;
        length = pkt.size() - 16;
    }

    uint32_t data_packet_context::get_sequence() const{
        return packet_sequence_number;
    }

    uint32_t data_packet_context::get_message_sequence() const{
        return message_number;
    }

    uint32_t data_packet_context::get_timestamp() const{
        return time_stamp;
    }

    uint32_t data_packet_context::get_socket_id() const{
        return socket_id;
    }

    bool data_packet_context::is_first_packet() const{
        return packet_position == 0b10;
    }

    bool data_packet_context::is_single_packet() const{
        return packet_position == 0b11;
    }

    bool data_packet_context::is_middle_packet() const{
        return packet_position == 0;
    }

    bool data_packet_context::is_last_packet() const{
        return packet_position == 1;
    }

    bool data_packet_context::should_order() const{
        return should_order_;
    }

    int data_packet_context::key_encrypt() const{
        return key_encrypt_;
    }

    bool data_packet_context::is_retransmitted() const{
        return is_retransmitted_;
    }

    const char* data_packet_context::data() const{
        return raw_data;
    }

    size_t data_packet_context::size() const{
        return length;
    }

    packet_context::packet_context(const srt_packet& pkt){
        char* data = const_cast<char*>(pkt.data());
        data_ptr = new (data) data_packet_context(pkt);
        control_ptr = new (data) control_packet_context(pkt);
    }

    const data_packet_context& packet_context::load_as_data() const {
        return *data_ptr;
    }

    const control_packet_context& packet_context::load_as_control() const {
        return *control_ptr;
    }

    bool ack::value(uint32_t val){

    }

    bool ack_ack::value(uint32_t val){

    }

};