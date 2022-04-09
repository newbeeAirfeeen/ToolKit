//
// Created by 沈昊 on 2022/4/8.
//

#ifndef TOOLKIT_SHARED_CONST_BUFFER_HPP
#define TOOLKIT_SHARED_CONST_BUFFER_HPP
#include <asio.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <ctime>

using asio::ip::tcp;

// A reference-counted non-modifiable buffer class.
class shared_const_buffer
{
public:
    // Construct from a std::string.
    explicit shared_const_buffer(const std::string& data)
        : data_(new std::vector<char>(data.begin(), data.end())),
          buffer_(asio::buffer(*data_))
    {
    }
    shared_const_buffer(const char* data, size_t length):data_(new std::vector<char>(data, data + length)),
            buffer_(asio::buffer(*data_)){}
    // Implement the ConstBufferSequence requirements.
    typedef asio::const_buffer value_type;
    typedef const asio::const_buffer* const_iterator;
    const asio::const_buffer* begin() const { return &buffer_; }
    const asio::const_buffer* end() const { return &buffer_ + 1; }
private:
    std::shared_ptr<std::vector<char> > data_;
    asio::const_buffer buffer_;
};

#endif//TOOLKIT_SHARED_CONST_BUFFER_HPP
