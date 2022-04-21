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
#include "socket_helper.hpp"
#include "spdlog/logger.hpp"
#include <memory>

template<typename _socket_type>
class basic_stream_oried_client: public socket_helper<_socket_type, basic_stream_oried_client<_socket_type>>,
                                 public std::enable_shared_from_this<basic_stream_oried_client<_socket_type>>{
public:
    using socket_type = _socket_type;
    using base_type = socket_helper<socket_type, basic_stream_oried_client<socket_type>>;
    using endpoint_type = typename base_type::endpoint_type;
public:

    explicit basic_stream_oried_client(event_poller& poller):base_type(poller){}

    std::shared_ptr<basic_stream_oried_client<socket_type>> shared_from_this_sub_type() final{
        return this->std::enable_shared_from_this<basic_stream_oried_client<socket_type>>::shared_from_this();
    }

    void async_connect(const endpoint_type& endpoint){
        auto stronger_self(shared_from_this_sub_type());
        base_type::getSock().async_connect(endpoint, [stronger_self, endpoint](const std::error_code& e){
            if(e){
                return stronger_self->onError(e);
            }
            stronger_self->begin_read();
            return stronger_self->onConnected();
        });
    }

    virtual void onConnected(){
        Info("connect success: {}:{}", base_type::getSock().remote_endpoint().address().to_string(), base_type::getSock().remote_endpoint().port());
    }

    void onRecv(buffer& buffer) override {
        Info("recv data length {}", buffer.size());
    }

    void onError(const std::error_code& e) override{
        Error("error: {}", e.message());
    }

    inline bool is_server(){
        return false;
    }
};

using tcp_client = basic_stream_oried_client<asio::ip::tcp::socket>;
#ifdef SSL_ENABLE
#include "ssl/tls.hpp"
#endif
#endif//TOOLKIT_CLIENT_HPP
