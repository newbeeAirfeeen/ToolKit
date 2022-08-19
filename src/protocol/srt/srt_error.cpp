﻿/*
* @file_name: srt_error.cpp
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
#include "srt_error.hpp"
#include "srt_handshake.h"
namespace srt {

    const char *srt_category::name() const noexcept {
        return "srt_error";
    }

    std::string srt_category::message(int err) const {
        switch (err) {
            case success:
                return "success";
            case status_error:
                return "srt status error, the srt operation is not permitted in this status";
            case srt_packet_error:
                return "srt packet error, current packet is not permitted with invalid arguments";
            case srt_control_type_error:
                return "srt control type error, current packet is not permitted with invalid arguments";
            case srt_stream_id_too_long:
                return "stream_id length is too long to load it";
            case srt_KM_REQ_is_not_support:
                return "at the this point, KE_REQ is not support";
            case srt_unknown_extension_field:
                return "unknown srt extension field";
            case srt_peer_error:
                return "peer has filesystem error";
            case srt_handshake_error:
                return "peer handshake parameters error";
            case too_large_payload:
                return "too large payload size, which is exceed MTU";
            case not_connected_yet:
                return "which operation is forbidden which connection is not established";
            case socket_write_error:
                return "socket write error";
            case socket_connect_time_out:
                return "connect time out";
            case socket_shutdown_op:
                return "active shutdown";
            case lost_peer_connection:
                return "peer has lost connection";
            case peer_has_terminated_connection:
                return "peer active terminated connection";
        }
        return "unknown";
    }

    const char *srt_reject_category::name() const noexcept {
        return "srt_reject_category";
    }


    std::string srt_reject_category::message(int err) const {
        return get_reject_reason(err);
    }


    std::error_category *generator_srt_category() {
        static srt_category c;
        return &c;
    }

    std::error_category *generator_srt_reject_category() {
        static srt_reject_category c;
        return &c;
    }

    std::error_code make_srt_error(int err) {
        return {err, *generator_srt_category()};
    }

    std::error_code make_srt_reject_error(int err) {
        return {err, *generator_srt_reject_category()};
    }
};// namespace srt