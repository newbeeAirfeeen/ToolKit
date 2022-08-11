/*
* @file_name: srt_client.hpp
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

#ifndef TOOLKIT_SRT_CLIENT_HPP
#define TOOLKIT_SRT_CLIENT_HPP

#include "srt_socket_service.hpp"

namespace srt {

    class srt_client {
    public:
        using endpoint_type = asio::ip::udp::endpoint;
        struct impl;

    public:
        explicit srt_client(asio::io_context &poller, const endpoint_type &host = {asio::ip::udp::v4(), 12000});
        void async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &)> &f);

    private:
        std::shared_ptr<impl> _impl;
    };
};// namespace srt

#endif//TOOLKIT_SRT_CLIENT_HPP
