/*
* @file_name: srt_error.hpp
* @date: 2022/04/26
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
#ifndef TOOLKIT_SRT_ERROR_HPP
#define TOOLKIT_SRT_ERROR_HPP
#include <string>
#include <system_error>
namespace srt {

    enum srt_error_code {
        success = 0,
        status_error = 1,
        srt_packet_error,/// srt 包解析失败
        srt_control_type_error,
        srt_stream_id_too_long,        /// stream_id 字段太长了
        srt_KM_REQ_is_not_support,     /// 握手阶段: 不支持的加密
        srt_unknown_extension_field,   /// 未知的扩展字段
        srt_peer_error,                /// 收到peer_error,
        srt_handshake_error,           /// 握手失败
        too_large_payload,             /// MTU 太大
        not_connected_yet,             /// 未完成连接,非法的操作。
        socket_write_error,            /// 发送数据失败
        socket_connect_time_out,       /// 连接超时
        socket_shutdown_op,            /// 主动断开连接
        lost_peer_connection,          /// 对端主动断开连接
        peer_has_terminated_connection,/// 对端主动关闭了连接
    };

    class srt_category : public std::error_category {
    public:
        const char *name() const noexcept override;
        std::string message(int err) const override;
    };

    class srt_reject_category : public std::error_category {
    public:
        const char *name() const noexcept override;
        std::string message(int err) const override;
    };

    std::error_category *generator_srt_category();
    std::error_category *generator_srt_reject_category();
    std::error_code make_srt_error(int err);
    std::error_code make_srt_reject_error(int err);
};// namespace srt

#endif//TOOLKIT_SRT_ERROR_HPP
