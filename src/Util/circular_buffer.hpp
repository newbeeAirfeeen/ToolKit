/*
* @file_name: circular_buffer.hpp
* @date: 2022/08/11
* @author: shen hao
* Copyright @ hz shen hao, All rights reserved.
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

#ifndef TOOLKIT_CIRCULAR_BUFFER_HPP
#define TOOLKIT_CIRCULAR_BUFFER_HPP
#include <cstdint>
#include <iterator>
#include <numeric>
#include <vector>
//// 实现环形缓冲和环形迭代器

template<typename T, typename alloc = std::allocator<T>>
class circular_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
public:
    circular_iterator(std::vector<T, alloc> *_target, uint64_t index) : target(_target), index(index) {}
    T &operator*() {
        return std::ref(target->operator[](index));
    }

    circular_iterator &operator=(const circular_iterator<T> &other) {
        target = other.target;
        index = other.index;
        return *this;
    }

    explicit circular_iterator(circular_iterator<T> &&other) {
        std::swap(target, other.target);
        index = other.index;
    }

    circular_iterator &operator++() {
        index = (index + 1) % target->size();
        return std::ref(target->operator[](index));
    }

    circular_iterator operator++(int) {
        auto tmp = *this;
        index = (index + 1) % target->size();
        return tmp;
    }

    bool operator!=(const circular_iterator<T, alloc> &other) {
        return index != other.index && other.target == target;
    }

private:
    std::vector<T, alloc> *target = nullptr;
    uint64_t index = 0;
};


template<typename T, typename allocator = std::allocator<T>>
class circular_buffer {
    friend class circular_iterator<T>;

public:
    using value_type = T;
    using iterator = circular_iterator<T>;
    using const_iterator = const iterator;
    using size_type = size_t;
    static constexpr const uint64_t MAX = (std::numeric_limits<uint64_t>::max)();
    template<typename TT>
    struct rebind_alloc {
        using type = std::allocator<TT>;
    };

public:
    explicit circular_buffer(size_t capacity = 8192) : max_size(capacity) {}

public:
    iterator begin() {
        return circular_iterator<T>(&queue, start_);
    }

    iterator end() {
        return circular_iterator<T>(&queue, end_);
    }

    const_iterator cbegin() {
        return std::cref(begin());
    }

    const_iterator cend() {
        return std::cref(end());
    }

    bool empty() const {
        return size_ == 0;
    }


    void emplace_back(T &&t) {
        if (size_ && start_ == end_) {
            start_ = (start_ + 1) % max_size;
            --size_;
        }
        if (queue.size() < max_size) {
            queue.emplace_back(std::move(t));
            end_ = queue.size();
        } else {
            end_ = (end_ + 1) % max_size;
            queue[end_] = std::move(t);
        }
        ++size_;
    }

    void push_back(const T &t) {
        return emplace_back(t);
    }

    T &front() {
        return queue[start_];
    }

    T &back() {
        return queue[end_ == 0 ? queue.size() - 1 : end_ - 1];
    }

    void pop_front() {
        if (!size_) {
            return;
        }
        start_ = (start_ + 1) % max_size;
        --size_;
    }

    size_type size() const {
        return size_;
    }

private:
    std::vector<T, allocator> queue;
    uint64_t max_size = 8192;
    uint64_t start_ = 0;
    uint64_t end_ = 0;
    uint64_t size_ = 0;
};

#endif//TOOLKIT_CIRCULAR_BUFFER_HPP
