//
// Created by 沈昊 on 2022/8/2.
//
#include <gtest/gtest.h>
#include <net/buffer.hpp>
#include <algorithm>

bool compare_string(const buffer &buf, const char *data, size_t length) {

    SCOPED_TRACE("compare_string scope");
    if (buf.size() != length) {
        throw std::runtime_error("the buf length is not equal to length");
    }

    decltype(length) index = 0;
    std::for_each(buf.cbegin(), buf.cend(), [&](const char &ch) {
        if (ch != buf[index]) {
            throw std::runtime_error("is not equal");
        }
        ++index;
    });
    return true;
}


void back_ward(buffer &buf) {
    SCOPED_TRACE("back_ward scope");
    EXPECT_EQ(buf.size(), 19);
    char buff[] = {0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00,
                   0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x07};
    EXPECT_NO_THROW(compare_string(buf, buff, 19));


    auto _1 = buf.get_be<uint16_t>();
    EXPECT_EQ(_1, 2);
    EXPECT_EQ(buf.size(), 17);

    auto _2 = buf.get_be<uint16_t>();
    EXPECT_EQ(_2, 4);
    EXPECT_EQ(buf.size(), 15);

    auto _3 = buf.get_be<uint16_t>();
    EXPECT_EQ(_3, 0);
    EXPECT_EQ(buf.size(), 13);


    auto _4 = buf.get_be<uint8_t>();
    EXPECT_EQ(_4, 5);
    EXPECT_EQ(buf.size(), 12);


    auto _6 = buf.get_be<uint32_t>();
    EXPECT_EQ(_6, 6);
    EXPECT_EQ(buf.size(), 8);

    auto _5 = buf.get_be<uint64_t>();
    EXPECT_EQ(_5, 7);
    EXPECT_EQ(buf.size(), 0);
}

TEST(set_and_get, buffer) {

    buffer buf;
    buf.reserve(1024);
    EXPECT_GT(buf.capacity(), static_cast<size_t>(1024));

    buf.put_be<uint16_t>(0x0002);
    buf.put_be<uint16_t>(0x0004);
    buf.put_be24(5);
    buf.put_be<uint32_t>(6);
    buf.put_be<uint64_t>(7);
    EXPECT_EQ(buf.size(), 19);
    char buff[] = {0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00,
                   0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x07};
    EXPECT_NO_THROW(compare_string(buf, buff, 19));
    /// backward
    back_ward(buf);
    buf.backward();
    back_ward(buf);
}