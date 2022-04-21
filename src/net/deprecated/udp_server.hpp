/*
* @file_name: udp_server.hpp
* @date: 2022/04/09
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
#ifndef TOOLKIT_UDP_SERVER_HPP
#define TOOLKIT_UDP_SERVER_HPP
#include "net/asio.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include "buffer.hpp"
#include "event_poller.hpp"
#include "context.hpp"
class udp_session;
class udp_server : public std::enable_shared_from_this<udp_server>, public asio::ip::udp::socket {
    friend class udp_session;
public:
    using pointer = std::shared_ptr<udp_server>;
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;
    using super_type = asio::ip::udp::socket;
public:
    explicit udp_server(event_poller& poller);
    /**
     * 开始一个udp套接字监听
     * @tparam session_type 继承于udp_session的session_type
     * @param port 端口
     * @param ip ipv4或者ipv6地址
     */
#ifdef SSL_ENABLE
    template<typename session_type>
    void start(unsigned short port, const std::string& ip = "0.0.0.0", const std::shared_ptr<context>& _context = nullptr){
        this->_context = _context;
#else
    template<typename session_type>
    void start(unsigned short port, const std::string& ip = "0.0.0.0"){
#endif
        endpoint_type endpoint(asio::ip::make_address(ip), port);
        super_type::open(endpoint.address().is_v4() ? asio::ip::udp::v4() : asio::ip::udp::v6());
        super_type::bind(endpoint);
        Info("udp server on {}:{}", ip, port);
        return read_datagram();
    }
    void stop();
private:
    void read_datagram();
    void transmit_message(basic_buffer<char>& buf, const endpoint_type& endpoint);
    void launchError(const std::error_code& e, const endpoint_type& endpoint);
    void remove_session(const endpoint_type& endpoint);
private:
    event_poller& poller;
    basic_buffer<char> buff;
    std::unordered_map<endpoint_type, std::shared_ptr<udp_session>> session_map;
#ifdef SSL_ENABLE
    std::shared_ptr<context> _context;
#endif
};




#endif//TOOLKIT_UDP_SERVER_HPP
