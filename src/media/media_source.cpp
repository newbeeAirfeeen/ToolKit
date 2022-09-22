/*
* @file_name: media_source.cpp
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
#include "media_source.hpp"
#include "net/executor_pool.hpp"
#include <mutex>
#include <unordered_map>
using pointer = typename media_source::pointer;
using ring_buffer_pointer = typename media_source::ring_buffer_pointer;
media_source::media_source() {
    _ring_buffer = std::make_shared<toolkit::RingBuffer<std::shared_ptr<buffer>>>();
}

const std::string &media_source::vhost() const {
    return _vhost;
}

const std::string &media_source::app() const {
    return _app;
}

const std::string &media_source::stream() const {
    return _stream;
}

void media_source::vhost(const string_view &_host_) {
    this->_vhost.assign(_host_.data(), _host_.size());
}

void media_source::app(const string_view &_app_) {
    this->_app.assign(_app_.data(), _app_.size());
}

void media_source::stream(const string_view &_stream_) {
    this->_stream.assign(_stream_.data(), _stream_.size());
}

const ring_buffer_pointer &media_source::get_ring() const {
    return _ring_buffer;
}

static std::unordered_map<std::string, std::unordered_map<size_t, pointer>> source_map;
static std::mutex source_mtx;
static size_t identify_l(const string_view &app, const string_view &stream) {
    constexpr uint64_t offset_basis = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;
    uint64_t val = offset_basis;
    auto func = [&](string_view view) {
        for (uint64_t idx = 0; idx < static_cast<uint64_t>(view.size()); ++idx) {
            val ^= static_cast<uint64_t>(view[idx]);
            val *= prime;
        }
    };
    func(app);
    func(stream);
    return static_cast<size_t>(val);
}

pointer media_source::add_or_remove_source(bool add, const pointer &p) {
    auto identify = identify_l(p->app(), p->stream());
    std::lock_guard<std::mutex> lmtx(source_mtx);
    auto it = source_map.find(p->vhost());
    if (add) {
        if (it == source_map.end()) {
            std::string v(p->vhost().data(), p->vhost().size());
            auto pair_ = source_map.insert(std::make_pair(v, std::unordered_map<size_t, pointer>()));
            it = pair_.first;
        }
        auto iter = it->second.find(identify);
        if (iter != it->second.end()) {
            return nullptr;
        }
        it->second.emplace(identify, p);
        return p;
    }
    if (it == source_map.end()) {
        return nullptr;
    }
    it->second.erase(identify);
    return nullptr;
}

pointer media_source::find_source(const string_view &vhost, const string_view &app, const string_view &stream) {
    auto identify = identify_l(app, stream);
    std::string _vhost(vhost.data(), vhost.size());
    std::lock_guard<std::mutex> lmtx(source_mtx);
    auto it = source_map.find(_vhost);
    if (it == source_map.end()) {
        return nullptr;
    }
    auto iter = it->second.find(identify);
    if (iter == it->second.end()) {
        return nullptr;
    }
    return iter->second;
}

void media_source::async_find_source(const string_view &vhost, const string_view &app, const string_view &stream, const std::function<void(const pointer &)> &f) {
    std::string _vhost(vhost.data(), vhost.size());
    std::string _app(app.data(), app.size());
    std::string _stream(stream.data(), stream.size());
    auto executor = executor_pool::instance().get_executor();
    executor->async([_vhost, _app, _stream, f]() {
        auto p = media_source::find_source(_vhost, _app, _stream);
        f(p);
    });
}

size_t media_source::identify(const string_view &app, const string_view &stream) {
    return identify_l(app, stream);
}