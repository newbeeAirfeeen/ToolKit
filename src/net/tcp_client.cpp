/*
* @file_name: tcp_client.hpp
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
#include "tcp_client.hpp"
#include "Util/onceToken.h"
#include "socket_base.hpp"
using super_type = typename tcp_client::super_type;

#ifdef SSL_ENABLE
tcp_client::tcp_client(bool current_thread, const std::shared_ptr<context> &_context_) : tcp_session(socket_helper::create_bind_socket(current_thread), _context_) {}
#else
tcp_client::tcp_client(bool current_thread) : tcp_session(socket_helper::create_bind_socket(current_thread)) {}
#endif

void tcp_client::on_connected() {
    Info("connected");
}

void tcp_client::onRecv(const char *data, size_t size) {
    Info("client recv {} length", size);
    send(data, size);
}

void tcp_client::bind_local(const std::string &ip, unsigned short port) {
    using endpoint = typename asio::ip::tcp::endpoint;
    auto address = asio::ip::make_address(ip);
    endpoint end_point(address, port);
    this->local_point = end_point;
}

void tcp_client::start_connect(const std::string &ip, unsigned short port) {
    toolkit::onceToken token([&]() { mtx.lock(); }, [&]() { mtx.unlock(); });
    close();
    open_l(this->local_point.address().is_v4());
    super_type::bind(this->local_point);
    endpoint end_point(asio::ip::make_address(ip), port);
    auto stronger_self(std::static_pointer_cast<tcp_client>(shared_from_this()));
    super_type::async_connect(end_point, [stronger_self](const std::error_code &e) {
        if (e) {
            Error(e.message());
            stronger_self->onError(e);
            return;
        }
        stronger_self->on_connected();
        return stronger_self->read_l();
    });
}
void tcp_client::send_loop(basic_buffer<char>& buf){
    auto stronger_self(shared_from_this());
    std::shared_ptr<basic_buffer<char>> buff = std::make_shared<basic_buffer<char>>(std::move(buf));
    get_poller().async([stronger_self, buff](){
        stronger_self->send(*buff);
    });
}