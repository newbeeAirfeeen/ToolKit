/*
* @file_name: srt_url.cpp
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
#include "srt_stream_id.hpp"
#include "Util/optional.hpp"
#include "Util/string_util.hpp"
#include "srt_error.hpp"
#include <vector>
namespace srt {
    using index_type = typename string_view::size_type;
    stream_id stream_id::from_buffer(string_view view) {
        index_type index = view.find("#!::");
        if (index != 0) {
            throw std::system_error(make_srt_error(srt_stream_serialize_error));
        }
        view.remove_prefix(4);
        auto vec = string_util::split(view, ",");
        if (vec.empty()) {
            throw std::system_error(make_srt_error(srt_stream_serialize_error));
        }
        stream_id _stream_id;
        for (const auto item: vec) {
            auto v = string_util::split(item, "=");
            if (v.size() != 2) {
                throw std::system_error(make_srt_error(srt_stream_serialize_error));
            }
            auto key = v.front();
            auto value = v.back();

            if (key.size() > 1) {
                std::string _key(key.data(), key.size());
                std::string _value(value.data(), value.size());
                _stream_id._query.emplace(_key, _value);
                continue;
            }
            auto ch = key[0];
            switch (ch) {
                case 'h':
                    v = string_util::split(v.back(), "/");
                    if (v.size() > 3) {
                        throw std::system_error(make_srt_error(srt_stream_serialize_error));
                    }
                    if (v.size() == 3) {
                        _stream_id.vhost(v.front());
                        v.pop_front();
                    }
                    if (v.size() >= 2) {
                        _stream_id.app(v.front());
                        _stream_id.stream(v.back());
                    } else {
                        _stream_id.vhost(v.front());
                    }
                    break;
                case 'r':
                    v = string_util::split(v.back(), "/");
                    if (vec.size() > 2) {
                        throw std::system_error(make_srt_error(srt_stream_serialize_error));
                    }
                    _stream_id.app(v.front());
                    _stream_id.stream(v.back());
                    break;
                case 'm': {
                    _stream_id._is_publish = "publish" == value;
                    break;
                }
            }
        }
        return _stream_id;
    }

    stream_id stream_id::from_buffer(const char *data, size_t length) {
        return stream_id::from_buffer(string_view(data, length));
    }

    std::string stream_id::to_buffer(const stream_id &_url) {
        std::string stream;
        stream.reserve(1024);
        stream.append("#!::h=");
        if (!_url.vhost().empty())
            stream.append(_url.vhost()).append("/");
        if (_url.app().empty() || _url.stream().empty()) {
            stream.append("live/stream");
        } else {
            stream.append(_url.app() + "/").append(_url.stream());
        }
        stream.append(",m=");
        if (_url.is_publish())
            stream.append("publish");
        else
            stream.append("request");
        for (const auto &query: _url._query) {
            stream.append(",");
            stream.append(query.first.data(), query.first.size());
            stream.append("=");
            stream.append(query.second.data(), query.second.size());
        }
        if (stream.size() > 728) {
            throw std::system_error(make_srt_error(srt_stream_id_too_long));
        }
        return stream;
    }

    void stream_id::is_publish(bool v) {
        this->_is_publish = v;
    }

    void stream_id::vhost(string_view v) {
        this->_vhost.assign(v.data(), v.size());
    }

    void stream_id::app(string_view v) {
        this->_app.assign(v.data(), v.size());
    }

    void stream_id::stream(string_view v) {
        this->_stream.assign(v.data(), v.size());
    }

    void stream_id::set_query(string_view k, const std::string &v) {
        _query.emplace(std::string(k.data(), k.size()), v);
    }


    bool stream_id::is_publish() const {
        return _is_publish;
    }

    const std::string &stream_id::vhost() const {
        return this->_vhost;
    }

    const std::string &stream_id::app() const {
        return this->_app;
    }

    const std::string &stream_id::stream() const {
        return this->_stream;
    }

    const std::string &stream_id::get_query(const std::string &v) const {
        static std::string str;
        auto it = _query.find(v);
        if (it != _query.end()) {
            return it->second;
        }
        return str;
    }

};// namespace srt
