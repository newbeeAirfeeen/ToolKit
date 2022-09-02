/*
* @file_name: srt_bandwidth.hpp
* @date: 2022/08/27
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

#ifndef TOOLKIT_SRT_BANDWIDTH_HPP
#define TOOLKIT_SRT_BANDWIDTH_HPP
#include <cstdint>
#include <mutex>
/***
 * To ensure smooth video playback on receiver peer during live stream,
 * SRT must control the sender's buffer level to prevent overfill and depletion.
 * The pacing control module is designed to
 * send packets as fast as they are submitted by a video application
 * while maintaining a relatively stable buffer level
 *
 * This balance is achieved by adjusting the maximum allowed bandwidth
 * MAX_BW (Section 5.1.1) which limits the bandwidth usage by SRT.
 *
 * During live streaming over highly variable networks, fairness can be
 * achieved by controlling the bitrate of the source encoder at the
 * input of the SRT sender. SRT sender can provide a variety of network
 * related statistics, such as RTT estimate, packet loss level, the
 * number of packets dropped, etc., to the encoder which can be used for
 * making decisions and adjusting the bitrate in real time.
 */

class bandwidth_mode {
public:
    virtual ~bandwidth_mode() = default;
    virtual void input_packet(uint16_t size) = 0;
    virtual void set_bandwidth(uint64_t);
    virtual uint64_t get_bandwidth() const;

protected:
    uint64_t &bandwidth();

private:
    uint64_t band_width = 1000000000 / 8;
};


class max_set_bandwidth_mode : public bandwidth_mode {
public:
    void input_packet(uint16_t size) override;
};

class constant_rate_mode : public bandwidth_mode {
public:
    void input_packet(uint16_t size) override;
    void set_bandwidth(uint64_t) override;
};


class estimated_bandwidth_mode : public bandwidth_mode {
public:
    estimated_bandwidth_mode();
    ~estimated_bandwidth_mode() override = default;
    void input_packet(uint16_t size) override;
    uint64_t get_bandwidth() const override;

private:
    mutable std::mutex mtx;
    uint64_t _last_input_time_point = 0;
    uint64_t bytes = 0;
};


#endif//TOOLKIT_SRT_BANDWIDTH_HPP
