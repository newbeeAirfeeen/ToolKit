/*
* @file_name: media_source.hpp
* @date: 2022/09/21
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

#ifndef TOOLKIT_MEDIA_SOURCE_HPP
#define TOOLKIT_MEDIA_SOURCE_HPP
#include "Util/RingBuffer.h"
#include "Util/nocopyable.hpp"
#include "Util/string_view.h"
#include "net/buffer.hpp"
#include "source_event.hpp"
#include <functional>
#include <memory>
class media_source : public source_event, public noncopyable, public std::enable_shared_from_this<media_source> {
public:
    using pointer = std::shared_ptr<media_source>;
    using ring_buffer_pointer = std::shared_ptr<toolkit::RingBuffer<std::shared_ptr<buffer>>>;

public:
    media_source();
    ~media_source() override = default;
    const std::string &vhost() const;
    const std::string &app() const;
    const std::string &stream() const;

    void vhost(const string_view &_host_);
    void app(const string_view &_app_);
    void stream(const string_view &_stream_);

public:
    const ring_buffer_pointer &get_ring() const;

public:
    static pointer add_or_remove_source(bool add, const pointer &p);
    static pointer find_source(const string_view &vhost, const string_view &app, const string_view &stream);
    static void async_find_source(const string_view &vhost, const string_view &app, const string_view &stream, const std::function<void(const pointer &)> &f);
    static size_t identify(const string_view &app, const string_view &stream);

private:
    std::string _vhost;
    std::string _app;
    std::string _stream;
    ring_buffer_pointer _ring_buffer;
};


#endif//TOOLKIT_MEDIA_SOURCE_HPP
