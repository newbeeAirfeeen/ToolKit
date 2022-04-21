/*
* @file_name: engine.hpp
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

#ifndef TOOLKIT_engine_HPP
#define TOOLKIT_engine_HPP
#include <Util/nocopyable.hpp>
#if defined(SSL_ENABLE)
#include <openssl/ssl.h>
#include <functional>
#include <vector>
#include <list>
#include <string>
#include <net/buffer.hpp>
class engine : public noncopyable{
public:
    /**
     * @description: 创建engnie对象
     * @param ctx OpenSSL context对象
     * @param server_mode 是否是服务器模式
     */
    engine(SSL_CTX* ctx, bool server_mode);

    ~engine();

    engine(engine&& other);

    engine& operator = (engine&& other);

    /**
     * @description: 当收到数据的时候
     * @param data 数据指针
     * @param length 数据长度
     */
    void onRecv(buffer& buff);
    /**
     * @description: 写入数据到ssl
     * @param data 数据指针
     * @param length 数据长度
     */
    void onSend(buffer& buff);

    /**
     * 出错回调
     * @param err_func 出错回调
     */
    void onError(const std::function<void()>& err_func);
public:
    /**
     * @description: 解密完成后的回调
     * @param f 回调函数
     */
    void setOnRecv(const std::function<void(buffer&)>& f);
    /**
     * @description: 加密完成后的回调函数
     * @param f 回调函数
     */
    void setOnWrite(const std::function<void(buffer&)>& f);
    void flush();
private:
    void shutdown();
    void flush_read_bio();
    void flush_write_bio();
private:
    SSL* _ssl;
    BIO* read_bio;
    BIO* write_bio;
    std::function<void(buffer&)> on_dec_func;
    std::function<void(buffer&)> on_enc_func;
    std::function<void()> err_func;
    bool send_handshake = false;
    bool server_mode = true;
    bool is_flush = false;
    std::list<buffer> _buffer_send_;
};
#endif
#endif//TOOLKIT_engine_HPP
