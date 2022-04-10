/*
* @file_name: buffer.hpp
* @date: 2021/10/06
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
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#pragma once
#ifndef OAHO_BUFFER_H
#define OAHO_BUFFER_H
#include <memory>
#include <string>
#include <type_traits>
struct big_endian_policy {
    using type = big_endian_policy;

    template<typename T>
    void operator()(T &val) {
        char *begin = (char *) std::addressof(val);
        constexpr int size = sizeof(T);
        constexpr int loop = size >> 1;
        for (int i = 0; i < loop; i++) std::swap(begin[i], begin[size - i - 1]);
    }
};

template<typename CharT, typename Traits = std::char_traits<CharT>,
         typename allocator = std::allocator<CharT>>
class basic_buffer : protected std::basic_string<CharT, Traits, allocator> {
public:
    using value_type = CharT;
    using base_type = std::basic_string<CharT, Traits, allocator>;
    using this_type = basic_buffer<CharT, Traits, allocator>;
    using Ptr = std::shared_ptr<basic_buffer<CharT, Traits, allocator>>;

public:
    inline static Ptr assign(const CharT *buf, size_t len) {
        return std::make_shared<this_type>(buf, len);
    }

    inline static Ptr assign() {
        return std::make_shared<this_type>();
    }

    inline static Ptr assign(std::string &&str) {
        return std::make_shared<this_type>(std::move(str));
    }

    inline static Ptr assign(basic_buffer<CharT, Traits, allocator> &&buf) {
        return std::make_shared<this_type>(std::move(buf));
    }

    basic_buffer() noexcept;

    basic_buffer(std::string &&str);

    basic_buffer(basic_buffer<CharT, Traits, allocator> &&) noexcept;


    basic_buffer(const CharT *, size_t) noexcept;

    size_t size() const;

    bool empty() const;

    void remove(size_t);

    const CharT *data() const;

    void clear();

    void append(const CharT *, size_t);

    void swap(basic_buffer<CharT, Traits, allocator> &);


    /* for base type, int, short */
    template<typename T, typename policy = big_endian_policy, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    basic_buffer &put(T val) {
        using put_type = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
        put_type v = val;
        policy p;
        p(v);
        std::basic_string<CharT, Traits, allocator>::append((const char *) &v, sizeof(put_type));
        return *this;
    }

private:
    size_t read_index;
};

template<typename CharT, typename Traits, typename allocator>
basic_buffer<CharT, Traits, allocator>::basic_buffer() noexcept : read_index(0) {}


template<typename CharT, typename Traits, typename allocator>
basic_buffer<CharT, Traits, allocator>::basic_buffer(std::string &&str) : read_index(0), std::basic_string<CharT, Traits, allocator>(std::move(str)) {}


template<typename CharT, typename Traits, typename allocator>
basic_buffer<CharT, Traits, allocator>::basic_buffer(basic_buffer<CharT, Traits, allocator> &&b) noexcept : base_type(std::move(b)) {
    read_index = b.read_index;
    b.read_index = 0;
}


template<typename CharT, typename Traits, typename allocator>
basic_buffer<CharT, Traits, allocator>::basic_buffer(const CharT *buf, size_t len) noexcept : read_index(0) {
    base_type::append(buf, len);
}

template<typename CharT, typename Traits, typename allocator>
inline size_t basic_buffer<CharT, Traits, allocator>::size() const { return base_type::size() - read_index; }


template<typename CharT, typename Traits, typename allocator>
inline bool basic_buffer<CharT, Traits, allocator>::empty() const { return size() == 0; }


template<typename CharT, typename Traits, typename allocator>
inline void basic_buffer<CharT, Traits, allocator>::remove(size_t len) {
    if (len > size()) {
        len = size();
    }
    read_index += len;
    if (read_index > 10240) {
        base_type::erase(0, read_index);
        read_index = 0;
    }
    if (read_index >=  base_type::size() - 1) {
        read_index = 0;
        base_type::clear();
    }
    if (base_type::capacity() > 40240) base_type::shrink_to_fit();
}

template<typename CharT, typename Traits, typename allocator>
inline void basic_buffer<CharT, Traits, allocator>::append(const CharT *buf, size_t len) {
    base_type::append(buf, len);
}

template<typename CharT, typename Traits, typename allocator>
inline const CharT *basic_buffer<CharT, Traits, allocator>::data() const {
    return base_type::data() + read_index;
}

template<typename CharT, typename Traits, typename allocator>
inline void basic_buffer<CharT, Traits, allocator>::clear() {
    base_type::clear();
    base_type::shrink_to_fit();
    read_index = 0;
}


template<typename CharT, typename Traits, typename allocator>
inline void basic_buffer<CharT, Traits, allocator>::swap(basic_buffer<CharT, Traits, allocator> &other) {
    base_type::swap(other);
    std::swap(read_index, other.read_index);
}

using buffer = basic_buffer<char>;

#endif