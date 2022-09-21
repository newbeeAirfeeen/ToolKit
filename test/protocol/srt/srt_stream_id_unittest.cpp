//
// Created by 沈昊 on 2022/9/20.
//
#include "protocol/srt/srt_stream_id.hpp"
#include <gtest/gtest.h>
TEST(srt_stream_id, serialize_1) {
    const char *stream_id = "#!::h=live/test,m=publish";
    auto stream_id_ = srt::stream_id::from_buffer(stream_id);
    EXPECT_EQ(stream_id_.vhost(), "");
    EXPECT_EQ(stream_id_.app(), "live");
    EXPECT_EQ(stream_id_.stream(), "test");
    EXPECT_EQ(stream_id_.is_publish(), true);

    auto str = srt::stream_id::to_buffer(stream_id_);
    EXPECT_EQ(str, stream_id);


    stream_id = "#!::h=www.baidu.com.cn/live/livestream,m=publish,username=shenhao,password=123";
    stream_id_ = srt::stream_id::from_buffer(stream_id);
    EXPECT_EQ(stream_id_.vhost(), "www.baidu.com.cn");
    EXPECT_EQ(stream_id_.app(), "live");
    EXPECT_EQ(stream_id_.stream(), "livestream");
    EXPECT_EQ(stream_id_.is_publish(), true);
    EXPECT_EQ(stream_id_.get_query("username"), "shenhao");
    EXPECT_EQ(stream_id_.get_query("password"), "123");

    str = srt::stream_id::to_buffer(stream_id_);
    EXPECT_EQ(str.size(), strlen(stream_id));
}