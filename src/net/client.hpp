/*
* @file_name: client.hpp
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
#include <Util/nocopyable.hpp>
#include <event_poller.hpp>
#include <asio/ssl.hpp>
#include "buffer.hpp"
#include <Util/onceToken.h>
#include <asio.hpp>
#include <mutex>
#include "socket_base.hpp"
#include "session.hpp"
template<typename _stream_type>
class client : public session<_stream_type>{
public:
    using stream_type = _stream_type;
    using endpoint = typename asio::ip::tcp::endpoint;
    using base_type = session<stream_type>;
public:
#ifdef SSL_ENABLE
    client(event_poller& poller, const std::shared_ptr<asio::ssl::context>& context)
        :session<_stream_type>(socket_helper::create_bind_socket(), context){
        this->context = context;
    }
#else
    explicit client(event_poller& poller):_sock(poller.get_executor()), base_type(_sock, poller){}
#endif
    virtual void on_connected(){
        Info("connected");
    }

    void bind_local(const std::string& ip, unsigned short port){
        using endpoint = typename asio::ip::tcp::endpoint;
        auto address = asio::ip::make_address(ip);
        endpoint end_point(address, port);
        this->local_point = end_point;
    }

    void start_connect(const std::string& ip, unsigned short port){
        toolkit::onceToken token([&](){ mtx.lock();}, [&](){mtx.unlock();});
        stream_type::close();
        stream_type::open(local_point.address().is_v4());
        stream_type::bind(this->local_point);
        endpoint end_point(asio::ip::make_address(ip) ,port);
        auto stronger_self(std::static_pointer_cast<client<stream_type>>(base_type::shared_from_this()));
        stream_type::async_connect_l(end_point, [stronger_self](const std::error_code& e){
            if(e){
                Error(e.message());
                stronger_self->onError(e);
                return;
            }
            stronger_self->on_connected();
            return stronger_self->read_l();
        });
    }

private:
    std::recursive_mutex mtx;
    endpoint local_point;
#ifdef SSL_ENABLE
    std::shared_ptr<asio::ssl::context> context;
#endif
};


#endif//TOOLKIT_CLIENT_HPP
