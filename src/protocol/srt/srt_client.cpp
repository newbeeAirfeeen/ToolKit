/*
* @file_name: srt_client.cpp
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
#include "event_poller_pool.hpp"
#include "impl/srt_client_impl.hpp"
namespace srt {

    srt_client::srt_client(const endpoint_type &host) {
        _impl = std::make_shared<srt_client::impl>(event_poller_pool::Instance().get_poller(false), host);
        _impl->begin();
    }

    void srt_client::set_max_payload(uint16_t length) {
        _impl->set_max_payload(length);
    }

    void srt_client::set_max_window_size(uint16_t size) {
        _impl->set_max_flow_window_size(size);
    }

    void srt_client::set_enable_drop_late_packet(bool on) {
        _impl->set_drop_too_late_packet(on);
    }

    void srt_client::set_enable_report_nack(bool on) {
        _impl->set_report_nak(on);
    }

    void srt_client::set_stream_id(const std::string &stream_id) {
        _impl->set_stream_id(stream_id);
    }

    void srt_client::set_connect_timeout(uint32_t ms) {
        _impl->set_connect_timeout(ms);
    }

    void srt_client::set_max_receive_time_out(uint32_t ms) {
        _impl->set_max_receive_time_out(ms);
    }

    uint16_t srt_client::get_max_payload() const {
        return (uint16_t) _impl->get_max_payload();
    }

    uint16_t srt_client::get_max_window_size() const {
        return (uint16_t) _impl->get_max_flow_window_size();
    }

    bool srt_client::enable_drop_too_late_packet() const {
        return (uint16_t) _impl->get_drop_too_late_packet();
    }

    bool srt_client::enable_report_nack() const {
        return _impl->get_report_nak();
    }

    const std::string &srt_client::stream_id() const {
        return _impl->get_stream_id();
    }

    uint32_t srt_client::socket_id() const {
        return _impl->get_sock_id();
    }

    uint32_t srt_client::connect_timeout() const {
        return _impl->get_connect_timeout();
    }

    uint32_t srt_client::max_receive_time_out() const {
        return _impl->get_max_receive_time_out();
    }

    void srt_client::async_connect(const endpoint_type &remote, const std::function<void(const std::error_code &e)> &f) {
        _impl->async_connect(remote, f);
    }
    void srt_client::set_on_error(const std::function<void(const std::error_code &)> &f) {
        return _impl->set_on_error(f);
    }
    void srt_client::set_on_receive(const std::function<void(const std::shared_ptr<buffer> &)> &f) {
        return _impl->set_on_receive(f);
    }
    int srt_client::async_send(const char *data, size_t length) {
        return _impl->async_send(data, length);
    }
};// namespace srt
