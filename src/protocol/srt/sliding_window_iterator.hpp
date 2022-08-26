/*
* @file_name: sender_queue.hpp
* @date: 2022/08/23
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

#ifndef TOOLKIT_SLIDING_WINDOW_ITERATOR_HPP
#define TOOLKIT_SLIDING_WINDOW_ITERATOR_HPP
#include <iterator>
#include <vector>
#include <functional>
template<typename type>
class sliding_iterator : public std::iterator<std::forward_iterator_tag, type> {
public:
    using size_type = size_t;
    sliding_iterator(std::vector<type> &target, size_type index, const size_type &_size) : _target(&target), _index(index), _size(_size) {}
    sliding_iterator &operator++() {
        _index = (_index + 1) % (*_target).size();
        if (_target->operator[](_index))
            --_size;
        return *this;
    }

    sliding_iterator operator++(int) {
        auto tmp = _index;
        auto size = _size;
        _index = (_index + 1) % (*_target).size();
        if (_target->operator[](_index))
            --_size;
        return sliding_iterator<type>(*_target, tmp, size);
    }

    type &operator*() {
        return std::ref(_target->operator[](_index));
    }

    type &operator->() {
        return std::ref(_target->operator[](_index));
    }


    bool operator!=(const sliding_iterator<type> &other) {
        if ((*_target) != (*other._target)) {
            return true;
        }

        if (_index != other._index) {
            return true;
        }

        if (_size != other._size) {
            return true;
        }
        return false;
    }


    bool operator==(const sliding_iterator<type> &other) const {
        return _index == other._index && (*_target) == (*other._target) && _size == other._size;
    }

private:
    std::vector<type> *_target = nullptr;
    size_type _index = 0;
    size_type _size;
};

template<typename T>
bool operator==(const sliding_iterator<T> &lhs, const sliding_iterator<T> &rhs) {
    return lhs.operator==(rhs);
}
#endif//TOOLKIT_SLIDING_WINDOW_ITERATOR_HPP
