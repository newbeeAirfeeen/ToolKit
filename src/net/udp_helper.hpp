/*
* @file_name: udp_helper.hpp
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
#ifndef TOOLKIT_UDP_HELPER_HPP
#define TOOLKIT_UDP_HELPER_HPP
#include "asio.hpp"
#include <unordered_set>
class udp_helper{
public:
    using endpoint_type = typename asio::ip::udp::socket::endpoint_type;
public:
    explicit udp_helper(asio::ip::udp::socket& sock);
    ~udp_helper();
    /**
     * 绑定一个本地地址,调用接口后,如果要再发送数据，需要再调用connect
     * @param port 端口
     * @param ip ipv4或者ipv6
     */
    void bind_local(unsigned short port, const std::string& ip = "0.0.0.0");
    /**
     * @descirption: 连接一个端点地址
     * @param endpoint
     */
    void connect(const endpoint_type& endpoint);
    /**
     * @description: 设置发送缓冲区大小
     * @param size size大小的字节
     */
    void set_send_buffer_size(size_t size);
    /**
     * @description: 设置接收缓冲区大小
     * @param size size大小的字节
     */
    void set_recv_buffer_size(size_t size);
    /**
     * @desciption: 是否发送广播包
     * @param use true表示使用广播
     */
    void use_broadcast(bool use);
    /**
     * @desciption: 加入一个多播组
     * @param ip ipv4或者ipv6
     */
    void join_multicast(const std::string &ip);
    /**
     * @desciption: 离开多播组
     * @param ip ipv4或者ipv6
     */
    void leave_multicast(const std::string &ip);
    /**
     * @description: 得到本机的地址
     * @return 本机绑定的地址
     */
    endpoint_type get_local_endpoint();
    /**
     * @description: 得到对端的地址
     * @return 对端的地址
     */
    endpoint_type get_remote_endpoint();
private:
    /**
     * @description: 内部调用,用于离开多播组.线程不安全
     */
    void leave_groups();
private:
    bool is_open = false;
    asio::ip::udp::socket& sock;
    /**
     * @description: 多播集
     */
    std::recursive_mutex mtx;
    std::unordered_set<std::string> multicast_set;
};

#endif//TOOLKIT_UDP_HELPER_HPP
