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

#include "asio.hpp"
#include "asio/basic_waitable_timer.hpp"
#include "buffer.hpp"
#include "event_poller.hpp"
#include "context.hpp"
#include "session_base.hpp"
#include "udp_helper.hpp"
#include "Util/nocopyable.hpp"
#include <chrono>
#include "socket_base.hpp"
class udp_server;
class udp_session : public udp_helper, public socket_sender<asio::ip::udp::socket, udp_session>, public std::enable_shared_from_this<udp_session>{
    friend class udp_server;

public:
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;
    using timer_type = asio::basic_waitable_timer<std::chrono::system_clock>;
public:
#ifdef SSL_ENABLE
    udp_session(event_poller &poller, const std::shared_ptr<context> &context, asio::ip::udp::socket &sock);
#else
    explicit udp_session(event_poller &poller, asio::ip::udp::socket &sock);
#endif
    ~udp_session();
    std::shared_ptr<udp_session> shared_from_this_subtype()override;
protected:
   void onRecv(const char *data, size_t size, const endpoint_type& endpoint);
   void onError(const std::error_code &e);
   void send(basic_buffer<char>& buffer, const endpoint_type& enpoint);
   void set_recv_time_out(size_t size);
private:
    void attach_server(udp_server *server);
    /**
     * @description:?????????????????????????????????????????????
     * @param buff ??????????????????
     * @param endpoint ??????????????????????????????
     */
    void launchRecv(basic_buffer<char> &buff, const endpoint_type& endpoint);
    /**
     * @description: ???udp_server??????
     */
    void begin_session();
    /**
     * @description: ???udp_server??????,?????????????????????
     */
    void flush();
private:
    /**
     * @description: ?????????udp_server
     */
    udp_server *server = nullptr;
    /**
     * @description: socket???????????????
     */
    asio::ip::udp::socket &sock;
    /**
     * @description: ?????????poller
     */
    event_poller &poller;
    /**
     * @description: ?????????????????????
     */
    basic_buffer<char> buffer_;
    /**
     * @description: ????????????
     */
    timer_type recv_timer;
    /**
     * @description: ????????????
     */
    std::atomic_size_t time_out{30};
    char buff[2048] = {0};
#ifdef SSL_ENABLE
    std::shared_ptr<context> _context;
#endif
};

#endif//TOOLKIT_UDPSESSION_HPP
