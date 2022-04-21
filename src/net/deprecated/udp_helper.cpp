/*
* @file_name: udp_helper.cpp
* @date: 2022/04/14
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
#include "udp_helper.hpp"
using endpoint_type = typename udp_helper::endpoint_type;


udp_helper::udp_helper(asio::ip::udp::socket& sock):sock(sock){}
udp_helper::~udp_helper(){

}

void udp_helper::bind_local(unsigned short port, const std::string& ip){
    auto endpoint = asio::ip::make_address(ip);
    sock.close();
    sock.open(endpoint.is_v4() ? asio::ip::udp::v4() : asio::ip::udp::v6());
    sock.bind(endpoint_type(endpoint, port));
}

void udp_helper::connect(const endpoint_type &endpoint) {
    sock.connect(endpoint);
}

void udp_helper::set_send_buffer_size(size_t size) {
    asio::socket_base::receive_buffer_size rbs(size);
    sock.set_option(rbs);
}
void udp_helper::set_recv_buffer_size(size_t size) {
    asio::socket_base::send_buffer_size sbs(size);
    sock.set_option(sbs);
}

void udp_helper::use_broadcast(bool use) {
    asio::socket_base::broadcast broadcast(use);
    sock.set_option(broadcast);
}

void udp_helper::join_multicast(const std::string &ip) {
    asio::ip::multicast::join_group group(asio::ip::make_address(ip));
    std::lock_guard<std::recursive_mutex> lmtx(mtx);
    sock.set_option(group);
    multicast_set.insert(ip);
}

void udp_helper::leave_multicast(const std::string &ip) {
    asio::ip::multicast::leave_group group(asio::ip::make_address(ip));
    std::lock_guard<std::recursive_mutex> lmtx(mtx);
    sock.set_option(group);
    multicast_set.erase(ip);
}
endpoint_type udp_helper::get_local_endpoint() {
    return sock.local_endpoint();
}

endpoint_type udp_helper::get_remote_endpoint() {
    return sock.remote_endpoint();
}

void udp_helper::leave_groups(){
    for (const auto &item: multicast_set) {
        asio::ip::multicast::leave_group group(asio::ip::make_address(item));
        sock.set_option(group);
    }
}