/*
* @file_name: tcp_client.hpp
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
#ifndef TOOLKIT_CLIENT_HPP
#define TOOLKIT_CLIENT_HPP
#include <memory>
#include <functional>
#include <event_poller.hpp>
#include "buffer.hpp"
#include <asio.hpp>
#include <net/ssl/context.hpp>
#include "tcp_session.hpp"
class tcp_client : public tcp_session{
public:
    using endpoint = typename asio::ip::tcp::endpoint;
public:
#ifdef SSL_ENABLE
    /**
     * @description: 建立一个tcp_client,
     * @param sock  socket,会移动构造
     * @param context 证书,用于建立tls
     */
    tcp_client(bool current_thread, const std::shared_ptr<context>& _context_ = nullptr);
#else
    explicit tcp_client(bool current_thread);
#endif
    /**
     * @description: 当连接建立的时候会调用的函数
     */
    virtual void on_connected();
    /**
     * 收到响应消息函数
     * @param data 数据指针
     * @param size 长度
     */
    void onRecv(const char* data, size_t size) override;
    /**
     * @description: 绑定本机ip和端口
     * @param ip ipv4或ipv6
     * @param port 端口
     */
    void bind_local(const std::string& ip, unsigned short port);

    /**
     * @description: 开始连接对应的ip和端口
     * @param ip ipv4或者ipv6
     * @param port 端口
     */
    void start_connect(const std::string& ip, unsigned short port);

public:
    void send_loop(basic_buffer<char>& buf);
private:
    std::recursive_mutex mtx;
    endpoint local_point;
};


#endif//TOOLKIT_CLIENT_HPP
