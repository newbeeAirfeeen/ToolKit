/*
* @file_name: srt_session.cpp
* @date: 2022/09/21
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
#include "srt_session.hpp"
#include "spdlog/logger.hpp"
namespace srt {
    srt_session::srt_session(const std::shared_ptr<asio::ip::udp::socket> &_sock, const event_poller::Ptr &context) : srt_session_base(_sock, context) {
    }

    srt_session::~srt_session() {
        Warn("~srt_session");
        if (_id.is_publish()) {
            media_source::add_or_remove_source(false, publish_source);
        }
    }

    void srt_session::onConnected() {
        try {
            const std::string &sid = get_stream_id();
            Info("peer stream id={}", sid);
            _id = srt::stream_id::from_buffer(sid.data(), sid.size());
        } catch (const std::system_error &e) {
            Error("{}", e.what());
            shutdown();
            return;
        }
        Info("stream id, vhost={}, app={}, stream={}, publish={}", _id.vhost(), _id.app(), _id.stream(), _id.is_publish());
        if (_id.is_publish()) {
            return handle_publish();
        }
        return handle_play();
    }

    void srt_session::onRecv(const std::shared_ptr<buffer> &buff) {
        if (!_id.is_publish() || !publish_source) {
            Warn("client not a publisher!, ignore the data packet");
            return;
        }
        ///Info("write {} bytes to ring buffer", buff->size());
        publish_source->get_ring()->write(buff);
    }

    void srt_session::onError(const std::error_code &e) {
        Error("error: {}", e.message());
    }

    void srt_session::handle_publish() {
        decltype(publish_source) source = std::make_shared<media_source>();
        source->vhost(_id.vhost());
        source->app(_id.app());
        source->stream(_id.stream());
        publish_source = media_source::add_or_remove_source(true, source);
        if (!publish_source) {
            Error("create media source failed, because {}/{}/{} media source already existed.", _id.vhost(), _id.app(), _id.stream());
            shutdown();
            return;
        }
    }

    void srt_session::handle_play() {
        std::weak_ptr<srt_session> self(std::static_pointer_cast<srt_session>(shared_from_this()));
        media_source::async_find_source(_id.vhost(), _id.app(), _id.stream(), [self](const media_source::pointer &p) {
            if (auto stronger_self = self.lock()) {
                return stronger_self->get_executor()->async([self, p]() {
                    if (auto stronger_self = self.lock()) {
                        return stronger_self->handle_play_l(p);
                    }
                });
            }
        });
    }

    void srt_session::handle_play_l(const media_source::pointer &p) {
        if (!p) {
            Error("no such stream={}/{} published, shutdown...", _id.app(), _id.stream());
            return shutdown();
        }
        play_source = p;
        std::weak_ptr<srt_session> self(std::static_pointer_cast<srt_session>(shared_from_this()));
        get_poller()->async([self, p]() {
            auto stronger_self = self.lock();
            if (!stronger_self) {
                return;
            }
            stronger_self->reader = p->get_ring()->attach(stronger_self->get_poller());
            stronger_self->reader->setDetachCB([self]() {
                if (auto stronger_self = self.lock()) {
                    return stronger_self->shutdown();
                }
            });

            stronger_self->reader->setReadCB([self](const std::shared_ptr<buffer> &buff) {
                if (auto stronger_self = self.lock()) {
                    stronger_self->async_send(buff);
                }
            });
        });
    }

}// namespace srt