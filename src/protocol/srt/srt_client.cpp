/*
* @file_name: srt_client.cpp
* @date: 2022/08/09
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
#include "event_poller_pool.hpp"
#include "impl/srt_client_impl.hpp"
namespace srt {

    srt_client::srt_client(const endpoint_type &host) {
        _impl = std::make_shared<srt_client::impl>(event_poller_pool::Instance().get_poller(false), host);
        _impl->begin();
    }

    void srt_client::async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &e)> &f) {
        _impl->async_connect(remote, f);
    }
    void srt_client::set_on_error(const std::function<void(const std::error_code &)> &f) {
        return _impl->set_on_error(f);
    }
    void srt_client::set_on_receive(const std::function<void(const std::shared_ptr<buffer> &)> &f) {
        return _impl->set_on_receive(f);
    }
    int srt_client::async_send(const char *data, size_t length) {
        return _impl->async_send(data, length);
    }
};// namespace srt
