/*
* @file_name: udp_client.hpp
* @date: 2022/04/13
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
#ifndef TOOLKIT_UDP_CLIENT_HPP
#define TOOLKIT_UDP_CLIENT_HPP
#include "asio.hpp"
#include "buffer.hpp"
#include "event_poller.hpp"
#include "socket_base.hpp"
#include "udp_helper.hpp"
#include <Util/nocopyable.hpp>
#include <net/ssl/context.hpp>
class udp_client : public udp_helper, public noncopyable, public std::enable_shared_from_this<udp_client>, public socket_sender<asio::ip::udp::socket, udp_client> {
public:
    using socket_type = asio::ip::udp::socket;
    using endpoint_type = asio::ip::udp::socket::endpoint_type;

public:
#ifdef SSL_ENABLE
    udp_client(event_poller &poller, const std::shared_ptr<context> &_context = nullptr);
#else
    explicit udp_client(event_poller &poller);
#endif
public:
    std::shared_ptr<udp_client> shared_from_this_subtype() override;
    /**
     * @description: 连接一个udp对端
     * @param endpoint
     */
    void connect(const endpoint_type &endpoint);
    /**
     * @description: 设置收到数据回调x
     * @param func 回调函数
     */
    void setOnRecv(const std::function<void(const char *, size_t, const endpoint_type &)> &func);
    /**
     * @description: 收到错误回调
     * @param func 错误回调
     */
    void setOnError(const std::function<void(const std::error_code &, const endpoint_type&)> &func);

    /**
     * 发送一个数据报，地址为先前connect的地址
     * @param buf 数据包
     */
    void send(basic_buffer<char>& buf);

private:
    void init();
    void read_l();

private:
    socket_type sock;
    event_poller &poller;
    char buff_[2048] = {0};
    /**
     * @description: 收到数据回调
     */
    std::function<void(const char *, size_t, const endpoint_type &)> on_recv_func;
    /**
     * @description: 出错回调
     */
    std::function<void(const std::error_code &)> on_error_func;
    std::recursive_mutex mtx;
};


#endif//TOOLKIT_UDP_CLIENT_HPP
