/*
* @file_name: srt_session.cpp
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
#include "srt_session.hpp"
#include "spdlog/logger.hpp"
namespace srt {
    srt_session::srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, const event_poller::Ptr &context) : srt_session_base(_sock, context) {
    }

    srt_session::~srt_session() {
        Warn("~srt_session");
    }

    void srt_session::onConnected() {
        try {
            const std::string &sid = get_stream_id();
            _id = srt::stream_id::from_buffer(sid.data(), sid.size());
        } catch (const std::system_error &e) {
            Error("{}", e.what());
            shutdown();
            return;
        }
        Info("stream id, vhost={}, app={}, stream={}, publish={}", _id.vhost(), _id.app(), _id.stream(), _id.is_publish());
    }

    void srt_session::onRecv(const std::shared_ptr<buffer> &) {
    }

    void srt_session::onError(const std::error_code &e) {
            Error("error: {}", e.message());
    }
}// namespace srt