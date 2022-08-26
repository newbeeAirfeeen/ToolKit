/*
* @file_name: packet_queue.hpp
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

#ifndef TOOLKIT_PACKET_QUEUE_HPP
#define TOOLKIT_PACKET_QUEUE_HPP
#include "net/buffer.hpp"
#include "sliding_window.hpp"

template<typename T>
class packet_queue : public sliding_window<T> {
public:
    using pointer = typename sliding_window<std::shared_ptr<buffer>>::pointer;
    using block_type = typename sliding_window<T>::block_type;
    using size_type = typename sliding_window<T>::size_type;
public:
    packet_queue(){
        _output_packet_func = [](const block_type &) {};
        _on_drop_packet_func = [](size_type, size_type) {};
    }
    ~packet_queue() override = default;

public:
    void set_output_packet(const std::function<void(const block_type &)> &f) {
        if (f) _output_packet_func = f;
    }

    void set_drop_packet(const std::function<void(size_type, size_type)> &f) {
        if (f) _on_drop_packet_func = f;
    }

private:
    ///
    void on_packet(const block_type &b) final {
        return _output_packet_func(b);
    }
    /// 主动丢包的范围
    void on_drop_packet(size_type begin, size_type end) final {
        return _on_drop_packet_func(begin, end);
    }

private:
    std::function<void(const block_type &)> _output_packet_func;
    std::function<void(size_type, size_type)> _on_drop_packet_func;
};

#endif//TOOLKIT_PACKET_QUEUE_HPP
