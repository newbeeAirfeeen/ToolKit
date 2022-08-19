/*
* @file_name: sender_queue.cpp
* @date: 2022/08/15
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
#include "sender_queue.hpp"

sender_queue::sender_queue(asio::io_context& context): sliding_window<std::shared_ptr<buffer>>(context){
    _output_packet_func = [](const block_type&){};
    _on_drop_packet_func = [](size_type, size_type){};
}

typename sender_queue::pointer sender_queue::get_shared_from_this() {
   return shared_from_this();
}

void sender_queue::on_packet(const block_type &b) {
    _output_packet_func(b);
}

void sender_queue::on_drop_packet(size_type begin, size_type end) {
    _on_drop_packet_func(begin, end);
}

void sender_queue::set_sender_output_packet(const std::function<void(const block_type &)> &f) {
    this->_output_packet_func = f;
}
void sender_queue::set_sender_on_drop_packet(const std::function<void(size_type, size_type)> &f) {
    this->_on_drop_packet_func = f;
}
