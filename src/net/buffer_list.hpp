/*
* @file_name: buffer_list.h
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
#ifndef AHUTIL_BUFFER_LIST_H
#define AHUTIL_BUFFER_LIST_H
#include "buffer.hpp"
#include <algorithm>
#include <asio/detail/buffer_sequence_adapter.hpp>
#include <list>
template<typename T, typename Traits = std::char_traits<T>, typename allocator = std::allocator<T>>
class basic_buffer_list : public std::list<basic_buffer<T>,
                                           typename std::allocator_traits<allocator>::template rebind_alloc<basic_buffer<T, Traits, allocator>>> {
public:
    using value_type = T;
    using this_type = basic_buffer_list<T, Traits, allocator>;
    using base_type = std::list<basic_buffer<T>, typename std::allocator_traits<allocator>::template rebind_alloc<basic_buffer<T, Traits, allocator>>>;

public:
    size_t remove(size_t remove_size) {
        if (base_type::empty()) {
            return 0;
        }
        auto origin_size = remove_size;
        while (!base_type::empty() && remove_size >= 0) {
            auto &front = base_type::front();
            if (front.size() > remove_size) {
                front.remove(remove_size);
                break;
            }
            remove_size -= front.size();
            base_type::pop_front();
        }
        return origin_size - remove_size;
    }

    size_t total_buffer_size() const {
        size_t buffers_size = 0;
        std::for_each(base_type::begin(), base_type::end(), [&](const basic_buffer<T> &buff) {
            buffers_size += buff.size();
        });
        return buffers_size;
    }

private:
};

/**
 * @description: 使用链表形式的缓冲区，在加入一串数据的时候，自动把内存管理权移动构造到链表尾部。
 *               避免string空间不够时，重新分配内存和拷贝的开销。
 *               再使用偏特化, 迫使asio开启合并写，由此减少系统调用的次数，以此来提高发送性能和吞吐量
 */
namespace asio {
    namespace detail {
        template<typename Buffer, typename T>
        class buffer_sequence_adapter<Buffer, basic_buffer_list<T>> : public buffer_sequence_adapter_base {
        public:
            enum { is_single_buffer = false };
            enum { is_registered_buffer = false };
            enum { linearisation_storage_size = 8192 };

        public:
            explicit buffer_sequence_adapter(const basic_buffer_list<T> &buffers)
                : count_(0), total_buffer_size_(0) {
                init(buffers);
            }

        public:
            native_buffer_type *buffers() {
                return buffer_s;
            }

            std::size_t count() const {
                return count_;
            }

            std::size_t total_size() const {
                return total_buffer_size_;
            }

            registered_buffer_id registered_id() const {
                return registered_buffer_id();
            }

            bool all_empty() const {
                return total_buffer_size_ == 0;
            }

            static bool all_empty(const basic_buffer_list<T> &buffers) {
                return buffers.empty();
            }

            static void validate(const basic_buffer_list<T> &buffers) {}


            static std::basic_string<T> first(const basic_buffer_list<T> &) {
                return {};
            }

        private:
            void init(const basic_buffer_list<T> &buffers) {
                auto iter = buffers.begin();
                auto end = buffers.end();
                for (; iter != end && count_ < max_buffers; ++iter, ++count_) {
                    total_buffer_size_ += iter->size();
#ifdef _WIN32
                    buffer_s[count_].buf = (char *) (iter->data());
                    buffer_s[count_].len = iter->size();
#else
                    buffer_s[count_] = {(void *) iter->data(), iter->size()};
#endif
                }
            }

        private:
            native_buffer_type buffer_s[max_buffers];
            std::size_t count_ = 0;
            std::size_t total_buffer_size_ = 0;
        };
    };// namespace detail
};    // namespace asio


#endif//AHUTIL_BUFFER_LIST_H
