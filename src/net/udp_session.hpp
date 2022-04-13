/*
* @file_name: udp_session.hpp
* @date: 2022/04/10
* @author: oaho
* Copyright @ hz oaho, All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#ifndef TOOLKIT_UDPSESSION_HPP
#define TOOLKIT_UDPSESSION_HPP

#include "udp.hpp"
#include <Util/nocopyable.hpp>
#include "session_base.hpp"
#include "event_poller.hpp"

template<typename _stream_type>
class datagram_session : public _stream_type, public basic_session, public noncopyable{
public:
    using stream_type = typename _stream_type::stream_type;
public:

private:
    event_poller& poller;
    char _buffer[2048] = {0};
};

using udp_session = udp::non_ssl<asio::ip::udp::socket>;
#if SSL_ENABLE
/*!
 * 这里需要扩展 暂时先
 */
using dtls_session = udp::ssl<asio::ip::udp::socket>;
#endif


#endif//TOOLKIT_UDPSESSION_HPP
