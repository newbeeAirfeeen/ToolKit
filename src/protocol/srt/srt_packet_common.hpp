/*
* @file_name: srt_packet_common.hpp
* @date: 2022/04/29
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
#ifndef TOOLKIT_SRT_PACKET_COMMON_HPP
#define TOOLKIT_SRT_PACKET_COMMON_HPP
#ifdef SRT_CORE_INTERNAL_DEBUG_LOG
#include "spdlog/logger.hpp"
#ifdef SRT_CORE_INTERNAL_DEBUG_LOG
#define SRT_TRACE_LOG(...) Trace(__VA_ARGS__)
#define SRT_DEBUG_LOG(...) Debug(__VA_ARGS__)
#define SRT_INFO_LOG(...) Info(__VA_ARGS__)
#define SRT_WARN_LOG(...) Warn(__VA_ARGS__)
#define SRT_ERROR_LOG(...) Error(__VA_ARGS__)
#else
#define SRT_TRACE_LOG(...)
#define SRT_DEBUG_LOG(...)
#define SRT_INFO_LOG(...)
#define SRT_WARN_LOG(...)
#define SRT_ERROR_LOG(...)
#endif
#endif
namespace srt{
    class srt_packet;
    class control_packet_context;
};


#endif//TOOLKIT_SRT_PACKET_COMMON_HPP
