/*
* @file_name: buffer.hpp
* @date: 2022/04/15
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
#include <ostream>
#include <asio/detail/buffer_sequence_adapter.hpp>
template<typename T>
struct return_type{
    using type = typename std::remove_reference<typename std::remove_pointer<typename std::remove_cv<T>::type>::type>::type;
    static constexpr const size_t size = sizeof(type);
};

template<typename CharT, typename Traits = std::char_traits<CharT>,
         typename allocator = std::allocator<CharT>>
class basic_buffer{
public:
    static constexpr auto npos = std::basic_string<CharT, Traits, allocator>::npos;
public:
    using value_type = CharT;
    using base_type = typename std::basic_string<CharT, Traits, allocator>;
    using traits_type = typename base_type::traits_type;
    using reference = typename base_type::reference;
    using const_reference = typename base_type::const_reference;
    using pointer = typename base_type::pointer;
    using const_pointer = typename base_type::const_pointer;
    using size_type = typename base_type::size_type;
    using this_type = basic_buffer<CharT, Traits, allocator>;
    using Ptr = std::shared_ptr<this_type>;
public:
    inline static Ptr assign(const CharT *buf, size_t len) {
        return std::make_shared<this_type>(buf, len);
    }

    inline static Ptr assign() {
        return std::make_shared<this_type>();
    }

    inline static Ptr assign(base_type& str){
        return std::make_shared<base_type>(std::move(str));
    }

    inline static Ptr assign(base_type&& str) {
        return std::make_shared<this_type>(std::move(str));
    }

    inline static Ptr assign(basic_buffer<CharT, Traits, allocator> &&buf) {
        return std::make_shared<this_type>(std::move(buf));
    }

    inline static Ptr assign(const base_type& str){
        return std::make_shared<this_type>(std::cref(str));
    }

    basic_buffer():read_index(0){}

    basic_buffer(const this_type& other):read_index(other.read_index),_data(other._data){}

    explicit basic_buffer(const CharT* data):_data(data), read_index(0){}

    explicit basic_buffer(const base_type& str):_data(str), read_index(0){}

    explicit basic_buffer(std::string &&str):read_index(0),_data(std::move(str)){}

    basic_buffer(basic_buffer<CharT, Traits, allocator> && other) noexcept{
        read_index = other.read_index;other.read_index = 0;
        _data = std::move(other._data);
    }

    basic_buffer(const CharT* data, size_t size) noexcept:read_index(0){
        _data.assign(data, size);
    }

    basic_buffer(std::initializer_list<CharT> il):_data(il), read_index(0){}

    template<typename InputIterator>
    basic_buffer(InputIterator begin, InputIterator end):_data(begin, end),read_index(0){}

    inline size_type size() const{
        return _data.size() - read_index;
    }

    inline bool empty() const{
        return size() == 0;
    }

    inline void backward(){
        read_index = 0;
    }

    void remove(size_t length){
        auto _size = size();
        if(!_size)return;
        read_index += ( length > _size ? _size : length);
        if( read_index >= 10240){
            _data.erase(0, read_index);
            read_index = 0;
        }
    }

    const_pointer data() const noexcept{
        return read_index + _data.data();
    }

    void clear() noexcept {
        read_index = 0;
        _data.clear();
    }

    void swap(basic_buffer<CharT, Traits, allocator>& other){
        std::swap(read_index, other.read_index);
        _data.swap(other._data);
    }

    void resize(size_t length){
        _data.resize(length);
        if(read_index >= length){
            read_index = _data.size();
        }
    }

    void reserve(size_t length){
        _data.reserve(length);
    }

    this_type& append(const base_type& str){
        _data.append(str);
        return *this;
    }

    this_type& append(const basic_buffer<value_type>& str){
        _data.append(str.data(), str.size());
        return *this;
    }

    this_type& append(const_pointer value){
        _data.append(value);
        return *this;
    }

    this_type& append(const_pointer value, size_t n){
        _data.append(value, n);
        return *this;
    }

    this_type& append(size_t n, value_type c){
        _data.append(n, c);
        return *this;
    }

    template<typename Iterator>
    this_type& append(Iterator begin, Iterator end){
        _data.append(begin, end);
        return *this;
    }

    this_type& append(std::initializer_list<value_type> il){
        _data.append(il);
        return *this;
    }

    reference& back(){
        return _data.back();
    }

    const_reference back() const{
        return _data.back();
    }

    const_pointer begin() noexcept{
        return _data.begin() + read_index;
    }

    size_type capacity() const noexcept{
        return _data.capacity();
    }

    const_pointer cbegin() const{
        return begin() + read_index;
    }

    const_pointer cend() const{
        return _data.end();
    }

    const_pointer end(){
        return _data.end();
    }

    int compare(const base_type& str) const noexcept{
        return _data.compare(read_index, str.size(), str);
    }

    int compare(size_type pos, size_type len, const base_type& str) const {
        return _data.compare( pos + read_index, len, str);
    }

    int compare(size_type pos, size_type len, const base_type& str, size_type sub_pos, size_type sub_len) const{
        return _data.compare(pos + read_index, len, str, sub_pos, sub_len);
    }

    int compare(const_pointer data_) const{
        return _data.compare(read_index, Traits::length(data_), data_);
    }

    int compare(size_type pos, size_type len, const_pointer data_) const {
        return _data.compare(pos + read_index, len, data_);
    }

    int compare(size_type pos, size_type len, const_pointer data_, size_type n) const {
        return _data.compare(pos + read_index, len, data_, n);
    }

    size_type copy(pointer s, size_type len, size_type pos = 0) const{
        return _data.copy(s, len , read_index + pos);
    }

    const_pointer c_str() const noexcept{
        return _data.data() + read_index;
    }

    this_type& erase(size_type pos = 0, size_type len = npos){
        _data.erase(pos + read_index, len);
        return *this;
    }

    size_type find(const base_type& str, size_type pos = 0) const noexcept{
        return _data.find(str, pos + read_index);
    }

    size_type find(const_pointer s, size_type pos = 0) const{
        return _data.find(s, pos + read_index);
    }

    size_t find (const char* s, size_t pos, size_type n) const{
        return _data.find(s, pos + read_index, n);
    }

    size_t find (char c, size_t pos = 0) const noexcept{
        return _data.find(c, pos + read_index);
    }

    this_type& operator += (const base_type& str){
        _data.operator += (str);
        return *this;
    }

    this_type& operator += (const_pointer s){
        _data.operator += (s);
        return *this;
    }

    this_type& operator += (value_type c){
        _data.operator += (c);
        return *this;
    }

    this_type& operator += (std::initializer_list<value_type> il){
        _data.operator += (il);
        return *this;
    }

    this_type& operator += (const this_type& str){
        _data.append(str.data(), str.size());
        return *this;
    }

   this_type& operator = (const base_type & str){
       read_index = 0;
       _data.operator = (str);
       return *this;
   }
   this_type& operator = (const char* s){
        read_index = 0;
        _data.operator = (s);
        return *this;
   }

   this_type& operator = (char c){
       read_index = 0;
       _data.operator=(c);
       return *this;
   }

   this_type& operator = (std::initializer_list<char> il){
       read_index = 0;
       _data.operator=(il);
       return *this;
   }

   this_type& operator = (base_type&& str) noexcept{
       read_index = 0;
       _data.operator=(std::forward<base_type>(str));
       return *this;
   }

   this_type& operator = (const this_type& str){
       if(this == std::addressof(str))
           return *this;
       read_index = str.read_index;
       _data.operator=(str);
       return *this;
   }

   reference operator[](size_type pos){
       return _data[pos + read_index];
   }

   const_reference operator[](size_type pos) const {
       return _data[pos + read_index];
   }

   friend std::ostream& operator << (std::ostream& os, const this_type& str){
       os.write(str.data(), str.size());
       return os;
   }

   size_type find_first_of(const base_type& str, size_type pos = 0) const noexcept{
       return _data.find_first_of(str, pos + read_index);
   }

   size_type find_first_of (const_pointer s, size_t pos = 0) const{
       return _data.find_first_of(s, pos + read_index);
   }

   size_type find_first_of (const_pointer s, size_t pos, size_t n) const{
       return _data.find_first_of(s, pos + read_index, n);
   }

   size_t find_first_of (char c, size_t pos = 0) const noexcept{
       return _data.find_first_of(c, pos + read_index);
   }


   unsigned int get_be24(){
     check_out_of_range(3);
     char buf[4] = {0};
     copy(buf + 1, 3);
     remove(3);
     unsigned int v = *(unsigned int*)(&buf);
     v = reverse(v);
     return v;
   }

   this_type& put_be24(unsigned int v){
     const char* p = (const char*)&v;
     char buf[3] = {p[2], p[1], p[0]};
     append(buf, 3);
     return *this;
   }

   template<typename T>
   typename return_type<T>::type get_be(){
       check_out_of_range(return_type<T>::size);
       typename return_type<T>::type v{0};
       copy((char*)(&v), return_type<T>::size);
       v = reverse(v);
       remove(return_type<T>::size);
       return v;
   }

   template<typename T>
   this_type& put_be(T v){
       typename return_type<T>::type _v = v;
       _v = reverse(_v);
       append((const char*)(&_v), return_type<T>::size);
       return *this;
   }
private:
    void check_out_of_range(size_type length){
        if(length > size())
            throw std::out_of_range("the need length is greater than size");
    }

    template<typename T>
    auto reverse(T v) -> typename return_type<T>::type {
        typename return_type<T>::type ret = v;
        char* pointer = (char*)(&ret);
        constexpr auto type_size = return_type<T>::size >> 1;
        for(int i = 0; i < type_size;i++){
            std::swap(pointer[i], pointer[return_type<T>::size - i - 1]);
        }
        return ret;
    }
private:
    size_t read_index;
    base_type _data;
};

template<typename CharT, typename Traits = std::char_traits<CharT>,
         typename allocator = std::allocator<CharT>>
class mutable_basic_buffer: public basic_buffer<CharT, Traits, allocator>{
public:
    char* data() const {
        return const_cast<char*>(basic_buffer<CharT, Traits, allocator>::data());
    }
};


namespace asio {
    namespace detail {
        template<typename Buffer, typename T>
        class buffer_sequence_adapter<Buffer, mutable_basic_buffer<T>> : public buffer_sequence_adapter_base {
        public:
            enum { is_single_buffer = true };
            enum { is_registered_buffer = false };

        public:
            explicit buffer_sequence_adapter(const mutable_basic_buffer<T> &buffers){
                init(buffers);
            }

        public:
            native_buffer_type *buffers() {
                return buffer_s;
            }

            std::size_t count() const {
                return 1;
            }

            std::size_t total_size() const {
                return size;
            }

            registered_buffer_id registered_id() const {
                return {};
            }

            bool all_empty() const {
                return size == 0;
            }

            static bool all_empty(const mutable_basic_buffer<T> &buffers) {
                return buffers.empty();
            }

            static void validate(const mutable_basic_buffer<T> &buffers) {}


            static Buffer first(const mutable_basic_buffer<T>& mtb) {
                return Buffer((void*)mtb.data(), mtb.size());
            }
        private:
            void init(const mutable_basic_buffer<T> &buffers) {
                buffer_s->iov_base = buffers.data();
                size = buffers.size();
                int a  = 1;
            }
        private:
            native_buffer_type* buffer_s;
            size_t size = {0};
        };
    };// namespace detail
};    // namespace asio


using buffer = basic_buffer<char>;

#endif