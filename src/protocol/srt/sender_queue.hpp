/*
* @file_name: sender_queue.hpp
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

#ifndef TOOLKIT_SENDER_QUEUE_HPP
#define TOOLKIT_SENDER_QUEUE_HPP
#include "net/buffer.hpp"
#include "sliding_window.hpp"

class sender_queue : public sliding_window<std::shared_ptr<buffer>>, public std::enable_shared_from_this<sender_queue> {
public:
    using pointer = typename sliding_window<std::shared_ptr<buffer>>::pointer;

public:
    explicit sender_queue(asio::io_context &context);
    ~sender_queue() override = default;

public:
    void set_sender_output_packet(const std::function<void(const block_type &)> &f);
    void set_sender_on_drop_packet(const std::function<void(size_type, size_type)> &f);

private:
    pointer get_shared_from_this() final;
    ///
    void on_packet(const block_type &) final;
    /// 主动丢包的范围
    void on_drop_packet(size_type begin, size_type end) final;

private:
    std::function<void(const block_type &)> _output_packet_func;
    std::function<void(size_type, size_type)> _on_drop_packet_func;
};

#endif//TOOLKIT_SENDER_QUEUE_HPP
