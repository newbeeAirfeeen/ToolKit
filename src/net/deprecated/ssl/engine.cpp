//
// Created by 沈昊 on 2022/4/12.
//
#include "engine.hpp"
#include "Util/onceToken.h"
#include <iostream>
#include "spdlog/logger.hpp"
#if defined(OPENSSL_ENABLE)
#include <openssl/err.h>
std::string get_error_string(unsigned long err = ::ERR_get_error()) {
    char buf[4096] = {0};
    ERR_error_string_n(err, buf, sizeof(buf));
    return {buf, std::char_traits<char>::length(buf)};
}
engine::engine(SSL_CTX *ctx, bool server_mode)
    : _ssl(SSL_new(ctx)), read_bio(nullptr), write_bio(nullptr), server_mode(server_mode) {

    if (!_ssl) {
        throw std::runtime_error("SSL_CTX pointer is nullptr");
    }
    read_bio = BIO_new(BIO_s_mem());
    write_bio = BIO_new(BIO_s_mem());
    /**
     * 设置读写bio
     */
    ::SSL_set_bio(_ssl, read_bio, write_bio);
    /**
     * 设置为服务器模式或者客户端模式
     */
    server_mode ? SSL_set_accept_state(_ssl) : SSL_set_connect_state(_ssl);
}

engine::~engine() {
    if (_ssl)
        ::SSL_free(_ssl);
}

engine::engine(engine &&other) {
    _ssl = other._ssl;
    other._ssl = nullptr;
    read_bio = other.read_bio;
    other.read_bio = nullptr;
    write_bio = other.write_bio;
    other.write_bio = nullptr;
    server_mode = other.server_mode;
}

engine &engine::operator=(engine &&other) {
    engine tmp_core(std::move(static_cast<engine &>(*this)));
    _ssl = other._ssl;
    other._ssl = nullptr;
    read_bio = other.read_bio;
    other.read_bio = nullptr;
    write_bio = other.write_bio;
    other.write_bio = nullptr;
    server_mode = other.server_mode;
    return *this;
}

void engine::onRecv(buffer& buf) {
    if (buf.size() <= 0) {
        return;
    }

    while (buf.size()) {
        /**
         * 首先把数据写入到_ssl绑定的read_bio的缓冲区中。
         */
        auto nwrite = BIO_write(read_bio, buf.data(), buf.size());
        if (nwrite > 0) {
            buf.remove(nwrite);
            /**
             * 尝试读取或者写出
             */
            flush();
            continue;
        }
        Error("ssl error: ", get_error_string());
        shutdown();
        break;
    }
}

void engine::onSend(buffer& buff){
    if(buff.size() <= 0)
        return;
    if(!server_mode && !send_handshake){
        send_handshake = true;
        SSL_do_handshake(_ssl);
    }
    _buffer_send_.emplace_back(std::move(buff));
    flush();
}

void engine::onError(const std::function<void()>& err_func){
    this->err_func = err_func;
}


void engine::shutdown() {
    _buffer_send_.clear();
    int ret = SSL_shutdown(_ssl);
    if( ret != 1){
        Error("ssl shutdown failed: {}", get_error_string());
    }else{
        flush();
    }
    if(err_func)
        err_func();
}
void engine::flush() {

    if (is_flush) return;
    toolkit::onceToken token([&] { is_flush = true; }, [&] { is_flush = false; });
    /**
     * 尝试从ssl中的读缓冲区读入数据
     */
    flush_read_bio();
    if (!SSL_is_init_finished(_ssl) || _buffer_send_.empty()) {
        /**
         * 未握手结束，尝试发送握手包
         */
        flush_write_bio();
        return;
    }

    while(!_buffer_send_.empty()){
        auto& front = _buffer_send_.front();
        size_t offset = 0;
        while(offset < front.size()){
            /**
             * 需要加密数据
             */
            auto nwrite = SSL_write(_ssl, front.data() + offset, front.size() - offset);
            if(nwrite > 0){
                offset += nwrite;
                flush_write_bio();
                continue;
            }
            break;
        }

        if(offset != front.size()){
            //未消费完毕,
            Error("ssl error: {}", get_error_string());
            shutdown();
            break;
        }
        _buffer_send_.pop_front();
    }
}

void engine::flush_read_bio() {
    char buffer_[32 * 1024] = {0};
    auto capacity = sizeof(buffer_) - 1;
    int nread = 0;
    size_t offset = 0;
    do {
        /**
         * 尝试从_ssl中读出数据
         */
        nread = SSL_read(_ssl, buffer_ + offset, capacity - offset);
        if (nread > 0) {
            offset += nread;
        }
    } while (nread > 0 && capacity - offset > 0);
    if (!offset) {
        return;
    }
    buffer_[offset] = '\0';
    buffer buff(buffer_, offset);
    /**
     * 此时有数据并调用回调
     */
    if (on_dec_func) {
        on_dec_func(buff);
    }
    /**
     * 如果还有数据,在尝试刷新
     */
    if (nread > 0) {
        return flush_read_bio();
    }
}

void engine::flush_write_bio() {
    char buffer_[32 * 1024] = {0};
    auto capacity = sizeof(buffer_) - 1;
    int nread = 0;
    size_t offset = 0;
    do {
        /**
         * 从ssl的write_bio中读取数据
         */
        nread = BIO_read(write_bio, buffer_ + offset, capacity - offset);
        if (nread > 0) {
            offset += nread;
        }
    } while (nread > 0 && capacity - offset > 0);
    if (!offset) {
        return;
    }
    buffer_[offset] = '\0';
    buffer buff(buffer_, offset);
    /**
     * 如果此时有数据并设置了回调，调用回调
     */
    if (on_enc_func) {
        on_enc_func(buff);
    }
    if (nread > 0) {
        return flush_write_bio();
    }
}

void engine::setOnRecv(const std::function<void(buffer&)> &rf) {
    this->on_dec_func = rf;
}

void engine::setOnWrite(const std::function<void(buffer&)>& wf){
    this->on_enc_func = wf;
}

#endif