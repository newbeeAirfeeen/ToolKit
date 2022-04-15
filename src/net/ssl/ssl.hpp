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
#ifndef TOOLKIT_SSL_HPP
#define TOOLKIT_SSL_HPP
#ifdef SSL_ENABLE
#include "engine.hpp"
#include "context.hpp"
#include <net/event_poller.hpp>
#include <net/buffer.hpp>
template<typename session_type>
class ssl : public session_type{
public:
    template<typename Arg>
    ssl(Arg&& arg, event_poller& poller, const std::shared_ptr<context>& context)
        :_engine(context->native_handle(), session_type::is_server()), session_type(std::forward<Arg>(arg), poller, context){
        _engine.setOnRecv(std::bind(&ssl<session_type>::self_onRecv, this, std::placeholders::_1,std::placeholders::_2));
        _engine.setOnWrite(std::bind(&ssl<session_type>::self_send, this, std::placeholders::_1, std::placeholders::_2));
        _engine.onError(std::bind(&ssl<session_type>::self_onErr, this));
    }

    template<typename Arg>
    ssl(Arg&& arg, const std::shared_ptr<context>& context): session_type(std::forward<Arg>(arg), context)
        ,_engine(context->native_handle(), false){
        _engine.setOnRecv(std::bind(&ssl<session_type>::self_onRecv, this, std::placeholders::_1,std::placeholders::_2));
        _engine.setOnWrite(std::bind(&ssl<session_type>::self_send, this, std::placeholders::_1, std::placeholders::_2));
        _engine.onError(std::bind(&ssl<session_type>::self_onErr, this));
    }

    ~ssl(){
        _engine.flush();
    }
    void onRecv(const char* data, size_t length) override{
        _engine.onRecv(data, length);
    }
    void send(basic_buffer<char>& buff) override{
        _engine.onSend(buff.data(), buff.size());
    }

private:
    void self_onRecv(const char* data, size_t length){
        session_type::onRecv(data, length);
    }

    void self_send(const char* data, size_t length){
        basic_buffer<char> tmp(data, length);
        session_type::send(tmp);
    }
    void self_onErr(){
        session_type::get_sock().shutdown(asio::socket_base::shutdown_both);
    }
private:
    engine _engine;
};
#endif
#endif//TOOLKIT_SSL_HPP
