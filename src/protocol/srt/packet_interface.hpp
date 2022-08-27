﻿/*
* @file_name: packet_interface.hpp
* @date: 2022/08/26
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
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

#ifndef TOOLKIT_PACKET_INTERFACE_HPP
#define TOOLKIT_PACKET_INTERFACE_HPP
#include <cstdint>
#include <memory>
#include <vector>
#include <utility>
template<typename T>
struct packet{
    uint32_t seq = 0;
    uint64_t submit_time = 0;
    bool is_retransmit = false;
    T pkt;
};

template<typename T>
class packet_interface{
public:
    using packet_pointer = std::shared_ptr<packet<T>>;
public:
    virtual ~packet_interface() = default;
public:
    packet_interface(){
        _on_drop_packet_func_ = [](uint32_t, uint32_t){};
        _on_packet_func_ = [](const packet_pointer&){};
    }
    void set_max_delay(uint32_t ms) {
        this->_max_delay = ms;
    }

    virtual void set_window_size(uint32_t size) {
        this->_window_size = size;
    }

    void set_max_sequence(uint32_t seq) {
        this->_max_sequence = seq;
    }

    uint32_t get_max_sequence() const {
        return this->_max_sequence;
    }

    uint32_t get_window_size() const{
        return this->_window_size;
    }

    uint32_t get_max_delay() const {
        return this->_max_delay;
    }

    uint32_t capacity() const {
        return _window_size - get_buffer_size();
    }

    virtual void set_current_sequence(uint32_t) = 0;
    virtual uint32_t get_current_sequence() const = 0;
    virtual uint32_t get_buffer_size() const = 0;
    virtual packet_pointer get_first_block() const = 0;
    virtual packet_pointer get_last_block() const = 0;
    virtual void input_packet(const T& t, uint32_t seq, uint64_t time_point) = 0;
    virtual void drop(uint32_t seq_begin, uint32_t seq_end) = 0;
    virtual void clear() = 0;
    virtual void on_packet(const packet_pointer& p) = 0;
    virtual void on_drop_packet(uint32_t begin, uint32_t end) = 0;
    void set_on_packet(const std::function<void(const packet_pointer&)>& f){
        if(f){
            this->_on_packet_func_ = f;
        }
    }

    void set_on_drop_packet(const std::function<void(uint32_t,uint32_t)>& f){
        if(f){
            this->_on_drop_packet_func_ = f;
        }
    }
protected:
    virtual bool is_cycle() const {
        if(!get_buffer_size()){
            return false;
        }
        auto begin = get_first_block()->seq;
        auto end = get_last_block()->seq;
        return ( begin > end ) && ( begin - end > (get_max_sequence() >> 1));
    }
    uint32_t get_time_latency(){
        if(!get_buffer_size() || !get_max_delay()){
            return 0;
        }
        auto first = get_first_block()->seq;
        auto last = get_last_block()->seq;
        uint32_t latency = 0;
        if(last > first){
            latency = last - first;
        }
        else{
            latency = first - last;
        }

        return latency > 0x80000000 ? (0xFFFFFFFF - latency) : latency;
    }
    std::function<void(uint32_t,uint32_t)> _on_drop_packet_func_;
    std::function<void(const packet_pointer&)> _on_packet_func_;
private:
    uint32_t _max_delay = 0;
    uint32_t _window_size = 8192;
    uint32_t _max_sequence = 0xFFFFFFFF;
};

template<typename T>
class packet_send_interface : public packet_interface<T>{
public:
    ~packet_send_interface() override = default;
    virtual void send_again(uint32_t begin, uint32_t end) = 0;
};

template<typename T>
class packet_receive_interface : public packet_interface<T>{
public:
    ~packet_receive_interface() override = default;
    virtual uint32_t get_expected_size() const = 0;
    virtual std::vector<std::pair<uint32_t, uint32_t>> get_pending_packets() const = 0;
};


#endif//TOOLKIT_PACKET_INTERFACE_HPP