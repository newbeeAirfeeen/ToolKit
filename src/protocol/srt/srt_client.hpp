/*
* @file_name: srt_client.hpp
* @date: 2022/08/09
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

#ifndef TOOLKIT_SRT_CLIENT_HPP
#define TOOLKIT_SRT_CLIENT_HPP
#include "srt_socket_service.hpp"
namespace srt {

    class srt_client {
    public:
        using endpoint_type = asio::ip::udp::endpoint;
        class impl;

    public:
        explicit srt_client(const endpoint_type &host = {asio::ip::udp::v4(), 0});
        /**
         * @description 设置最大MTU, 最大不超过1500
         * @default     默认值为1500
         * @param length MTU length
         */
        void set_max_payload(uint16_t length);
        /**
         * @desciption 设置滑动窗口大小
         * @default    默认窗口大小为8192
         * @param size 滑动窗口 size
         */
        void set_max_window_size(uint16_t size);
        /**
         * @description 是否允许丢掉延迟过大的包
         * @default     默认为允许
         * @param on    true为允许,false禁止
         */
        void set_enable_drop_late_packet(bool on);
        /**
         * @description 是否允许开启nack
         * @default     默认为开启
         * @param on    true为开启，false为关闭
         */
        void set_enable_report_nack(bool on);
        /**
         * @description 设置流的stream_id
         * @param stream_id
         */
        void set_stream_id(const std::string &stream_id);
        /**
         * @description 设置连接超时时间,单位为毫秒
         * @default     3秒钟
         * @param ms    单位为ms
         */
        void set_connect_timeout(uint32_t ms);
        /**
         * @description 设置读超时
         * @default     默认为10秒钟
         * @param ms    单位为ms
         */
        void set_max_receive_time_out(uint32_t ms);
        /**
         * @description 得到最大payload
         */
        uint16_t get_max_payload() const;
        /**
         * @description 得到滑动窗口大小
         */
        uint16_t get_max_window_size() const;
        /**
         * @description 是否允许丢包
         */
        bool enable_drop_too_late_packet() const;
        /**
         * @description 是否允许发送nack
         */
        bool enable_report_nack() const;
        /**
         * @description 流的stream_id
         */
        const std::string &stream_id() const;
        /**
         * @description 流的socket_id
         */
        uint32_t socket_id() const;
        /**
         * @description 连接超时
         */
        uint32_t connect_timeout() const;
        /**
         * @description 最大读超时
         */
        uint32_t max_receive_time_out() const;

        void async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &e)> &f);
        void set_on_error(const std::function<void(const std::error_code &)> &f);
        void set_on_receive(const std::function<void(const std::shared_ptr<buffer> &)> &f);
        int async_send(const char *data, size_t length);

    private:
        std::shared_ptr<impl> _impl;
    };
};// namespace srt

#endif//TOOLKIT_SRT_CLIENT_HPP
