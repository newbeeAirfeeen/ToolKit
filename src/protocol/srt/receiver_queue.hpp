/*
* @file_name: receiver_queue.hpp
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

#ifndef TOOLKIT_RECEIVE_QUEUE_HPP
#define TOOLKIT_RECEIVE_QUEUE_HPP
#include "sliding_window.hpp"
#include "srt_packet.h"

class receiver_queue : public sliding_window<std::shared_ptr<srt::srt_packet>>, public std::enable_shared_from_this<receiver_queue> {
public:
    using pointer = typename sliding_window<std::shared_ptr<srt::srt_packet>>::pointer;

public:
    explicit receiver_queue(asio::io_context &context);
    ~receiver_queue() override = default;

private:
    pointer get_shared_from_this() final;
    void on_packet(const block_type &type) final;
    void on_drop_packet(size_type begin, size_type end) final;
};


#endif//TOOLKIT_RECEIVE_QUEUE_HPP
