/*
* @file_name: tcp_session.hpp
* @date: 2022/04/21
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

#include "socket_helper.hpp"
#include "spdlog/logger.hpp"
class tcp_session : public socket_helper<asio::ip::tcp::socket, tcp_session>, public std::enable_shared_from_this<tcp_session>{
    friend class tcp_server;
public:
    template<typename...Arg>
    tcp_session(event_poller& poller, asio::ip::tcp::socket& sock, Arg&&...arg)
        : socket_helper<asio::ip::tcp::socket, tcp_session>(poller, sock, std::forward<Arg>(arg)...){}

    ~tcp_session(){
        Error("~tcp_session");
    }

    inline bool is_server(){
        return true;
    }

    void onRecv(buffer& buffer_) override {
        Info("recv data length {}", buffer_.size());
        buffer b("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nyes", 67);
        async_send(b);
    }

    void onError(const std::error_code& e) override{
        Error("error: {}", e.message());
    }
protected:

    std::shared_ptr<tcp_session> shared_from_this_sub_type() final{
        return shared_from_this();
    }

    virtual void onConnected(){

    }


};


#endif//TOOLKIT_TCP_SESSION_HPP
