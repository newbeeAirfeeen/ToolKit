/*
* @file_name: srt_error.hpp
* @date: 2022/04/26
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
#ifndef TOOLKIT_SRT_ERROR_HPP
#define TOOLKIT_SRT_ERROR_HPP
#include <string>
#include <system_error>
namespace srt {

    enum srt_error_code {
        status_error,
        srt_packet_error,
        srt_control_type_error,
        srt_stream_id_too_long,
        srt_KM_REQ_is_not_support,
        srt_unknown_extension_field,
    };

    class srt_category : public std::error_category {
    public:
        const char *name() const noexcept override;
        std::string message(int err) const override;
    };

    class srt_error : public std::system_error {
    public:
        explicit srt_error(std::error_code);
    };

    std::error_category *generator_srt_category();
    std::error_code make_srt_error(int err);
};// namespace srt

#endif//TOOLKIT_SRT_ERROR_HPP
