//
// Created by 沈昊 on 2022/4/12.
//
#include "engine.hpp"
#include "Util/onceToken.h"
#include "context.hpp"
#include <iostream>
#include <spdlog/logger.hpp>
#if defined(SSL_ENABLE) && defined(USE_OPENSSL)
engine::engine(SSL_CTX *ctx, bool server_mode)
    : _ssl(SSL_new(ctx)), read_bio(nullptr), write_bio(nullptr), server_mode(server_mode) {

    if (!_ssl) {
        throw std::runtime_error("SSL_CTX pointer is nullptr");
    }
    read_bio = BIO_new(BIO_s_mem());
    write_bio = BIO_new(BIO_s_mem());
    ::SSL_set_bio(_ssl, read_bio, write_bio);
    server_mode ? SSL_set_accept_state(_ssl) : SSL_set_connect_state(_ssl);
}

engine::~engine() {
    if (_ssl)
        ::SSL_free(_ssl);
//    if (read_bio)
//        ::BIO_free(read_bio);
//    if (write_bio)
//        ::BIO_free(write_bio);
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

void engine::onRecv(const char *data, size_t length) {
    if (length <= 0) {
        return;
    }

    size_t offset = 0;
    while (offset < length) {
        //把数据写入到read_bio中
        auto nwrite = BIO_write(read_bio, data + offset, length - offset);
        if (nwrite > 0) {
            offset += nwrite;
            flush();
            continue;
        }
        Error("ssl error: ", get_error_string());
        shutdown();
        break;
    }
}

void engine::onSend(const char* data, size_t length){
    if(length <= 0)
        return;
    if(!server_mode && !send_handshake){
        send_handshake = true;
        SSL_do_handshake(_ssl);
    }
    _buffer_send_.emplace_back(data, length);
    flush();
}

void engine::shutdown() {
    _buffer_send_.clear();
    int ret = SSL_shutdown(_ssl);
    if( ret != 1){
        Error("ssl shutdown failed: {}", get_error_string());
    }else{
        flush();
    }
}
void engine::flush() {

    if (is_flush) return;
    toolkit::onceToken token([&] { is_flush = true; }, [&] { is_flush = false; });

    flush_read_bio();
    if (!SSL_is_init_finished(_ssl) || _buffer_send_.empty()) {
        //未握手结束
        flush_write_bio();
        return;
    }

    while(!_buffer_send_.empty()){
        auto& front = _buffer_send_.front();
        size_t offset = 0;
        while(offset < front.size()){
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
    char buffer[32 * 1024] = {0};
    auto capacity = sizeof(buffer) - 1;
    int nread = 0;
    size_t offset = 0;
    do {
        nread = SSL_read(_ssl, buffer + offset, capacity - offset);
        if (nread > 0) {
            offset += nread;
        }
    } while (nread > 0 && capacity - offset > 0);
    if (!offset) {
        return;
    }
    buffer[offset] = '\0';
    if (on_dec_func) {
        on_dec_func(buffer, offset);
    }
    if (nread > 0) {
        return flush_read_bio();
    }
}

void engine::flush_write_bio() {
    char buffer[32 * 1024] = {0};
    auto capacity = sizeof(buffer) - 1;
    int nread = 0;
    size_t offset = 0;
    do {
        nread = BIO_read(write_bio, buffer + offset, capacity - offset);
        if (nread > 0) {
            offset += nread;
        }
    } while (nread > 0 && capacity - offset > 0);
    if (!offset) {
        return;
    }
    buffer[offset] = '\0';
    if (on_enc_func) {
        on_enc_func(buffer, offset);
    }
    if (nread > 0) {
        return flush_write_bio();
    }
}

void engine::setOnRecv(const std::function<void(const char *, size_t)> &rf) {
    this->on_dec_func = rf;
}

void engine::setOnWrite(const std::function<void(const char*, size_t)>& wf){
    this->on_enc_func = wf;
}

#endif