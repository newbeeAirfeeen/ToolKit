//
// Created by 沈昊 on 2022/4/8.
//

#ifndef TOOLKIT_SESSION_BASE_HPP
#define TOOLKIT_SESSION_BASE_HPP
#include <memory>
#include <system_error>
class basic_session : public std::enable_shared_from_this<basic_session>{
    friend class session_helper;
public:
    using pointer = std::shared_ptr<basic_session>;
public:
    /*!
     * 为数据接收回调
     * @param data
     * @param length
     */
    virtual void onRecv(const char* data, size_t length) = 0;
    /*!
     * 会话出错回调
     * @param e
     */
    virtual void onError(const std::error_code& e) = 0;
private:
    /*!
     * 用户标识
     */
    size_t identify;
};
#endif//TOOLKIT_SESSION_BASE_HPP
