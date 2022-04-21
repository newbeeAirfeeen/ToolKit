/*
* @file_name: tls.hpp
* @date: 2022/04/12
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
#ifndef TOOLKIT_TLS_HPP
#define TOOLKIT_TLS_HPP
#ifdef SSL_ENABLE
#include "engine.hpp"
#include "asio/ssl/context.hpp"
#include <net/event_poller.hpp>
#include <net/buffer.hpp>
#include "asio/basic_socket.hpp"
template<typename session_type>
class tls : public session_type{
public:
    tls(event_poller& poller, const std::shared_ptr<asio::ssl::context>& context)
        :_engine(context->native_handle(), session_type::is_server()), session_type(poller){

        _engine.setOnRecv(std::bind(&tls<session_type>::self_onRecv, this, std::placeholders::_1));
        _engine.setOnWrite(std::bind(&tls<session_type>::self_send, this, std::placeholders::_1));
        _engine.onError(std::bind(&tls<session_type>::self_onErr, this));
    }

    tls(event_poller& poller, asio::ip::tcp::socket& sock, const std::shared_ptr<asio::ssl::context>& context): session_type(poller, sock, context)
        ,_engine(context->native_handle(), session_type::is_server()){

        _engine.setOnRecv(std::bind(&tls<session_type>::self_onRecv, this, std::placeholders::_1));
        _engine.setOnWrite(std::bind(&tls<session_type>::self_send, this, std::placeholders::_1));
        _engine.onError(std::bind(&tls<session_type>::self_onErr, this));
    }

    ~tls(){
        _engine.flush();
    }
    void onRecv(buffer& buff) override{
        _engine.onRecv(buff);
    }
    void async_send(buffer& buff) override{
        _engine.onSend(buff);
    }

private:
    void self_onRecv(buffer& buff){
        session_type::onRecv(buff);
    }

    void self_send(buffer& buff){
        session_type::async_send(buff);
    }
    void self_onErr(){
        session_type::getSock().shutdown(asio::socket_base::shutdown_both);
    }
private:
    engine _engine;
};


#endif
#endif//TOOLKIT_TLS_HPP
