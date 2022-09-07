/*
* @file_name: executor_pool.hpp
* @date: 2022/09/07
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
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef TOOLKIT_EXECUTOR_POOL_HPP
#define TOOLKIT_EXECUTOR_POOL_HPP
#include "Util/nocopyable.hpp"
#include "executor.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class executor_pool : public noncopyable {
public:
    static executor_pool &instance();
    static void set_nums(size_t num);
    void for_each(const std::function<void(const std::shared_ptr<executor> &)> &f) {
        for (const auto &poller: executors) {
            auto func = std::bind(f, poller);
            poller->async(func);
        }
    }
    std::shared_ptr<executor> get_executor();

protected:
    executor_pool();

private:
    std::vector<std::shared_ptr<executor>> executors;
    static size_t num;
};


#endif//TOOLKIT_EXECUTOR_POOL_HPP
