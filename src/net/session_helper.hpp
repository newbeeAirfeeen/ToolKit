/*
* @file_name: session_helper.hpp
* @date: 2022/04/04
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
#ifndef TOOLKIT_SESSION_HELPER_HPP
#define TOOLKIT_SESSION_HELPER_HPP
#include <Util/nocopyable.hpp>
#include <unordered_map>
#include "session_base.hpp"
#include "asio.hpp"
#include <Util/onceToken.h>
class session_helper;
class session_helper : public noncopyable{
public:
    /*!
     * 创建会话，创建的会话会被加入调用此函数所在线程的session_helper对象
     * @return true表示创建成功
     */
    bool create_session(const basic_session::pointer&);
    /*!
     * 销毁会话，必须在会话绑定的线程操作不然可能会销毁别的会话！！
     * @return true表示销毁成功
     */
    bool remove_session(const basic_session::pointer&);
    /*!
     * 清除当前线程的所有会话
     * @return true
     */
    bool clear();
    /*!
     * 得到会话的总数量
     */
    static size_t get_session_count();
private:
    /*!
     * 会话个数
     */
    static std::atomic<size_t> session_count;
    static toolkit::onceToken token;
    std::unordered_map<typename asio::ip::tcp::socket::native_handle_type, typename basic_session::pointer> session_map;
};

session_helper& get_session_helper();
#endif//TOOLKIT_SESSION_HELPER_HPP
