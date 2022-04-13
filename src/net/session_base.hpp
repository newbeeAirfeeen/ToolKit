//
// Created by 沈昊 on 2022/4/8.
//

#ifndef TOOLKIT_SESSION_BASE_HPP
#define TOOLKIT_SESSION_BASE_HPP
#include <memory>
#include <system_error>
#include "buffer.hpp"
class basic_session : public std::enable_shared_from_this<basic_session>{
    friend class session_helper;
public:
    using pointer = std::shared_ptr<basic_session>;
public:
    virtual void onRecv(const char* data, size_t size) = 0;
    virtual void onError(const std::error_code &e) = 0;
    virtual void send(basic_buffer<char>& buffer) = 0;
private:
    /*!
     * 用户标识
     */
    size_t identify;
};
#endif//TOOLKIT_SESSION_BASE_HPP
