/*
* @file_name: srt_session.hpp
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

#ifndef TOOLKIT_SRT_SESSION_HPP
#define TOOLKIT_SRT_SESSION_HPP
#include "srt_session_base.hpp"
#include "srt_stream_id.hpp"
namespace srt {
    class srt_session : public srt_session_base {
    public:
        srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, const event_poller::Ptr &context);
        ~srt_session() override;

    protected:
        void onConnected() override;
        void onRecv(const std::shared_ptr<buffer> &) override;
        void onError(const std::error_code &e) override;

    private:
        typename srt::stream_id _id;
    };
};// namespace srt


#endif//TOOLKIT_SRT_SESSION_HPP
