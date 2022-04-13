/*
* @file_name: tcp_session.hpp
* @date: 2022/04/06
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

#ifndef TOOLKIT_TCP_SESSION_HPP
#define TOOLKIT_TCP_SESSION_HPP
#include "asio/basic_waitable_timer.hpp"
#include "buffer.hpp"
#include "event_poller.hpp"
#include "session_base.hpp"
#include "asio.hpp"
#ifdef SSL_ENABLE
#include "net/ssl/context.hpp"
#endif

class tcp_session : public basic_session, public asio::ip::tcp::socket{
    friend class tcp_server;
    friend class session_helper;

public:
    using pointer = std::shared_ptr<tcp_session>;
    using clock_type  = std::chrono::system_clock;
    using timer = asio::basic_waitable_timer<clock_type>;
    using super_type = asio::ip::tcp::socket;
public:
#ifdef SSL_ENABLE
    /**
     * @description: 用于tcp_server来创建一个会话
     * @param socket_ tcp_server创建的socket
     * @param poller  绑定的线程
     * @param context 证书,用于tls
     */
    tcp_session(super_type &socket_, event_poller &poller, const std::shared_ptr<context> &_context_);
    /**
     * @description: 创建一个与poller绑定的socket
     * @param pair_  socket绑定的poller, socket
     * @param _context_ 证书
     */
    tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>>& pair_, const std::shared_ptr<context>& _context_);
#else
    tcp_session(super_type &socket_, event_poller &poller);
    tcp_session(const std::pair<event_poller::Ptr, std::shared_ptr<asio::ip::tcp::socket>>& pair_);
#endif
    ~tcp_session();

    event_poller &get_poller();
    /*!
     * 为数据接收回调
     * @param data 数据头指针
     * @param length 数据的长度
     */
    void onRecv(const char *data, size_t length) override;

    /*!
     * 会话出错回调
     * @param e 错误码
     */
    void onError(const std::error_code &e) override;
    /*!
     * 在会话开始前会被调用
     */
    virtual void before_begin_session();
    /*!
     * 设置发送超时, 0为没有超时时间
     * @param time 时间单位为毫秒
     */
    void set_send_time_out(size_t time);
    /*!
     * 设置接收时间超时,0为没有超时时间
     * @param time 时间单位为毫秒
     */
    void set_recv_time_out(size_t time);
    /*!
     * 设置接收缓冲区大小
     * @param size 接收缓冲区的大小
     */
    void set_recv_buffer_size(size_t size);
    /*!
     * 设置发送缓冲区的大小
     * @param size 发送缓冲区的大小
     */
    void set_send_buffer_size(size_t size);
    /*!
     * 设置发送低水位
     * @param size 设置发送低水位
     */
    void set_send_low_water_mark(size_t size);
    /*!
     * 设置接收低水位
     * @param 接收低水位
     */
    void set_recv_low_water_mark(size_t size);
    /*!
     * 是否关闭nagle算法
     * @param no_delay true关闭,false开启。
     */
    void set_no_delay(bool no_delay = true);
public:
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param buffer buffer
     */
    void send(basic_buffer<char>& buffer) override;
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param data 字符头指针
     * @param length 字节数据长度
     */
    void send(const char *data, size_t length);
    /*!
     * 数据发送出口。只允许在绑定线程调用
     * @param str basic_string
     */
    void send(std::string &str);
    /**
     * 是否是服务端会话
     * @return true表示服务端会话
     */
    bool is_server() const;
public:
    virtual void begin_session();
protected:
    void read_l();
    void write_l();
    void open_l(bool ipv4);
    void shutdown();
protected:
    bool _is_server = true;
    /*!
     * 用户读缓冲区
     */
    char buffer[10240] = {0};
    event_poller &poller;
    /*!
     * 用户写缓冲区
     */
    basic_buffer<char> _buffer;
    /*!
     * 超时定时器
     */
    timer recv_timer;
    timer send_timer;
    /*!
     * 发送时间超时
     */
    std::atomic_size_t send_time_out{0};
    /*!
     * 接收时间超时
     */
    std::atomic_size_t recv_time_out{30};
    /*!
     * ssl上下文
     */
#ifdef SSL_ENABLE
    std::shared_ptr<context> _context;
#endif
};

#endif//TOOLKIT_TCP_SESSION_HPP
