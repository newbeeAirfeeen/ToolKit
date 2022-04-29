/*
* @file_name: err.cpp
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
#include "err.hpp"
namespace srt{

    const char* srt_category::name() const noexcept{
        return "srt_error";
    }

    std::string srt_category::message(int err) const{
        switch (err) {
            case srt_error_code::status_error: return "srt status error, the srt operation is not permitted in this status";
            case srt_error_code::srt_packet_error: return "srt packet error, current packet is not permitted with invalid arguments";
            case srt_error_code::srt_control_type_error: return "srt control type error, current packet is not permitted with invalid arguments";
        }
        return "";
    }

    srt_error::srt_error(std::error_code e): std::system_error(e){}

    std::error_code make_srt_error(int err){
        std::error_code code(err, srt_category());
        return code;
    }
};