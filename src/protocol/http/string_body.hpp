/*
* @file_name: string_body.hpp
* @date: 2022/09/17
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

#ifndef TOOLKIT_STRING_BODY_HPP
#define TOOLKIT_STRING_BODY_HPP
#include <type_traits>
#include <cstdint>
namespace http{
    template<typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
    class basic_string_body {
    private:
        static_assert(std::is_integral<CharT>::value && sizeof(CharT) == 1, "CharT requirements not met");
    public:
        using value_type = std::basic_string<CharT, Traits, Allocator>;

    public:
        ///  When this body is used with @ref message::prepare_payload
        ///  the Content-Length will be set to the payload size, and
        ///  any chunked Transfer-Encoding will be removed.
        static uint64_t size(const value_type& body){
            return body.size();
        }

        class reader{
            value_type& body_;
        public:
            template<bool is_request, typename fields>
            explicit reader(value_type& b):body_(b){}


        };


        class writer{

        };
    };
};



#endif//TOOLKIT_STRING_BODY_HPP
