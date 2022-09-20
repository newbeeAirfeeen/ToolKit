/*
* @file_name: srt_url.hpp
* @date: 2022/09/20
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

#ifndef TOOLKIT_SRT_STREAM_ID_HPP
#define TOOLKIT_SRT_STREAM_ID_HPP
#include "Util/string_view.h"
#include <unordered_map>
namespace srt {
    class stream_id {
    public:
        static stream_id from_buffer(string_view view);
        static stream_id from_buffer(const char *data, size_t length);
        static std::string to_buffer(const stream_id &_url);

    public:
        void is_publish(bool);
        void vhost(string_view);
        void app(string_view);
        void stream(string_view);
        void set_query(string_view, const std::string &);

        bool is_publish() const;
        const std::string &vhost() const;
        const std::string &app() const;
        const std::string &stream() const;
        const std::string &get_query(const std::string &v) const;

    private:
        bool _is_publish = false;
        std::string _vhost;
        std::string _app;
        std::string _stream;
        std::unordered_map<std::string, std::string> _query;
    };
};// namespace srt


#endif//TOOLKIT_SRT_STREAM_ID_HPP
