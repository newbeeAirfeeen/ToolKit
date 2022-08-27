//
// Created by 沈昊 on 2022/8/14.
//
#include <gtest/gtest.h>
#include <protocol/srt/sliding_window.hpp>
class basic_window : public sliding_window<int>, public std::enable_shared_from_this<basic_window> {
public:
    void on_packet(const block_type &type) override {
    }

    void on_drop_packet(size_type begin, size_type end) override {
    }

public:
    void send_to(int v) {
        auto b = std::make_shared<block>();
        b->content = v;
        sliding_window<int>::send_in(b);
    }

private:
};
#include <atomic>

void push_operation(const std::shared_ptr<basic_window> &window) {
    auto fun = [&](size_t val, int *arr) {
        window->send_to((int)val);
        auto begin = window->get_flow_part_begin();
        auto end = window->get_flow_part_end();
        size_t pos = 0;
        while (begin != end) {
            if ((*begin))
                EXPECT_EQ(arr[pos++], (*begin)->content);
            ++begin;
        }
        EXPECT_EQ(window->get_buffer_size(), 6);
        EXPECT_EQ(window->capacity(), 0);
    };
    window->clear();
    EXPECT_EQ(window->capacity(), 6);
    window->send_to(11);
    window->send_to(22);
    window->send_to(33);
    window->send_to(44);
    window->send_to(55);

    int s = 0;
    int ss[] = {11, 22, 33, 44, 55};
    auto b = window->get_flow_part_begin();
    auto e = window->get_flow_part_end();
    EXPECT_EQ(1, window->capacity());
    while (b != e) {
        EXPECT_EQ(ss[s++], (*b)->content);
        b++;
    }

    window->send_to(66);
    EXPECT_EQ(window->capacity(), 0);
    auto begin = window->get_flow_part_begin();
    auto end = window->get_flow_part_end();

    int buf[] = {22, 33, 44, 55, 66, 77};
    fun(77, buf);
    EXPECT_EQ(window->get_first_block()->content, 22);
    EXPECT_EQ(window->get_last_block()->content, 77);


    int buf1[] = {33, 44, 55, 66, 77, 88};
    fun(88, buf1);
    EXPECT_EQ(window->get_first_block()->content, 33);
    EXPECT_EQ(window->get_last_block()->content, 88);


    int buf2[] = {44, 55, 66, 77, 88, 99};
    fun(99, buf2);
    EXPECT_EQ(window->get_first_block()->content, 44);
    EXPECT_EQ(window->get_last_block()->content, 99);

    int buf3[] = {55, 66, 77, 88, 99, 100};
    fun(100, buf3);
    EXPECT_EQ(window->get_first_block()->content, 55);
    EXPECT_EQ(window->get_last_block()->content, 100);

    int buf4[] = {66, 77, 88, 99, 100, 110};
    fun(110, buf4);
    EXPECT_EQ(window->get_first_block()->content, 66);
    EXPECT_EQ(window->get_last_block()->content, 110);

    int buf5[] = {77, 88, 99, 100, 110, 120};
    fun(120, buf5);
    EXPECT_EQ(window->get_first_block()->content, 77);
    EXPECT_EQ(window->get_last_block()->content, 120);

    int buf6[] = {88, 99, 100, 110, 120, 1};
    fun(1, buf6);
    EXPECT_EQ(window->get_first_block()->content, 88);
    EXPECT_EQ(window->get_last_block()->content, 1);

    int buf7[] = {99, 100, 110, 120, 1, 2};
    fun(2, buf7);
    EXPECT_EQ(window->get_first_block()->content, 99);
    EXPECT_EQ(window->get_last_block()->content, 2);

    int buf8[] = {100, 110, 120, 1, 2, 3};
    fun(3, buf8);
    EXPECT_EQ(window->get_first_block()->content, 100);
    EXPECT_EQ(window->get_last_block()->content, 3);

    int buf9[] = {110, 120, 1, 2, 3, 4};
    fun(4, buf9);
    EXPECT_EQ(window->get_first_block()->content, 110);
    EXPECT_EQ(window->get_last_block()->content, 4);

    int buf11[] = {120, 1, 2, 3, 4, 5};
    fun(5, buf11);
    EXPECT_EQ(window->get_first_block()->content, 120);
    EXPECT_EQ(window->get_last_block()->content, 5);

    int buf12[] = {1, 2, 3, 4, 5, 6};
    fun(6, buf12);
    EXPECT_EQ(window->get_first_block()->content, 1);
    EXPECT_EQ(window->get_last_block()->content, 6);

    int buf13[] = {2, 3, 4, 5, 6, 1212};
    fun(1212, buf13);
    EXPECT_EQ(window->get_first_block()->content, 2);
    EXPECT_EQ(window->get_last_block()->content, 1212);
}

void find_operation(const std::shared_ptr<basic_window> &window) {
    window->clear();
    window->set_window_size(6);
    window->set_initial_sequence(3);
    window->set_max_sequence(16);
    /// seq: 3 4 5 6 7 8
    /// val: 0 1 2 3 4 5
    for (int i = 0; i < 6; i++) {
        window->send_to(i);
    }

    auto it = window->find_block_by_sequence(4);
    EXPECT_EQ(it->content, 1);
    auto it_2 = window->find_block_by_sequence(3);
    EXPECT_EQ(0, it_2->content);
    auto it_3 = window->find_block_by_sequence(8);
    EXPECT_EQ(it_3->content, 5);
    auto it_4 = window->find_block_by_sequence(9);
    EXPECT_EQ(it_4, window->get_flow_part_end());


    int arr[] = {1, 2, 3, 4, 5};
    int i = 0;
    while (it != window->get_flow_part_end()) {
        EXPECT_EQ((*it)->content, arr[i++]);
        ++it;
    }
    EXPECT_EQ(5, i);
    int arr2[] = {0, 1, 2, 3, 4, 5};
    i = 0;
    while (it_2 != window->get_flow_part_end()) {
        EXPECT_EQ((*it_2)->content, arr2[i++]);
        it_2++;
    }
    EXPECT_EQ(i, 6);

    /// seq: 9 4 5 6 7 8
    /// val: 6 1 2 3 4 5
    window->send_to(6);
    auto iter = window->find_block_by_sequence(6);
    EXPECT_EQ(iter->content, 3);


    /// seq: 9 10 11 12 13 14
    /// val: 6 22 33 44 55 66
    window->send_to(22);
    window->send_to(33);
    window->send_to(44);
    window->send_to(55);
    window->send_to(66);
    window->send_to(77);
    /// seq: 15  0 11 12 13 14
    /// val: 77 88 33 44 55 66
    window->send_to(88);

    auto first = window->get_first_block();
    auto last = window->get_last_block();
    EXPECT_EQ(first->content, 33);
    EXPECT_EQ(last->content, 88);

    auto _it = window->find_block_by_sequence(15);
    EXPECT_EQ(77, _it->content);

    auto _t = window->find_block_by_sequence(0);
    EXPECT_EQ(_t->content, 88);

    ///EXPECT_EQ(1,0);
    ++_t;
    EXPECT_EQ(_t, window->get_flow_part_end());
}


void sequence_to(const std::shared_ptr<basic_window> &window) {
    window->clear();
    window->set_window_size(6);
    window->set_initial_sequence(2);
    window->set_max_sequence(16);
    EXPECT_EQ(window->capacity(), 6);

    /// seq 2   3  4  5  6  7
    /// val 11 22 33 44 55 66
    window->send_to(11);
    window->send_to(22);
    window->send_to(33);
    window->send_to(44);
    window->send_to(55);
    window->send_to(66);

    EXPECT_EQ(window->capacity(), 0);
    EXPECT_EQ(window->get_buffer_size(), 6);
    window->sequence_to(1);
    window->sequence_to(2);
    window->sequence_to(3);
    window->sequence_to(4);
    EXPECT_EQ(window->get_buffer_size(), 4);
    EXPECT_EQ(window->get_first_block()->content, 33);
    EXPECT_EQ(window->get_last_block()->content, 66);

    window->sequence_to(7);
    EXPECT_EQ(window->get_buffer_size(), 1);
    EXPECT_EQ(window->get_first_block()->content, 66);
    EXPECT_EQ(window->get_last_block()->content, 66);
    window->sequence_to(8);
    EXPECT_EQ(window->get_buffer_size(), 0);

    window->clear();
    window->set_initial_sequence(2);
    window->set_window_size(6);
    window->set_max_sequence(16);
    /// seq 2   3  4  5  6  7
    /// val 11 22 33 44 55 66
    window->send_to(11);
    window->send_to(22);
    window->send_to(33);
    window->send_to(44);
    window->send_to(55);
    window->send_to(66);

    window->send_to(77);
    window->send_to(88);
    /// 8   9  4  5  6  7
    /// 77 88 33 44 55 66
    window->send_to(99);
    window->send_to(110);
    ///  8  9  10  11  6  7
    /// 77  88 99  110 55 66
    window->send_to(120);
    window->send_to(130);
    ///  8  9  10  11  12  13
    /// 77  88 99  110 120 130
    window->send_to(140);
    window->send_to(150);
    window->send_to(1);
    window->send_to(2);
    window->send_to(3);
    /// 14  15  0  1  2  13
    /// 140 150 1  2  3 130


    EXPECT_EQ(130, window->get_first_block()->content);
    EXPECT_EQ(13, window->get_first_block()->sequence_number);
    EXPECT_EQ(2, window->get_last_block()->sequence_number);
    EXPECT_EQ(3, window->get_last_block()->content);

    window->sequence_to(1);
    EXPECT_EQ(2, window->get_buffer_size());
    EXPECT_EQ(1, window->get_first_block()->sequence_number);
    EXPECT_EQ(2, window->get_first_block()->content);
    EXPECT_EQ(3, window->get_last_block()->content);
    EXPECT_EQ(2, window->get_last_block()->sequence_number);
}


void receiver_queue(const std::shared_ptr<basic_window> &window) {

    window->clear();
    window->set_window_size(1000);
    window->set_max_sequence(100);
    window->set_initial_sequence(30);
    window->set_max_delay(0);


    window->arrived_packet(32, 0, -1);
    window->arrived_packet(35, 0, -1);
    window->arrived_packet(36, 0, -1);
    window->arrived_packet(37, 0, -1);
    window->arrived_packet(38, 0, -1);
    window->arrived_packet(40, 0, -1);
    window->arrived_packet(50, 0, -1);
    window->arrived_packet(52, 0, -1);
    window->arrived_packet(54, 0, -1);
    window->arrived_packet(56, 0, -1);
    std::vector<std::pair<uint32_t, uint32_t>> v{
            {30, 31},
            {33, 34},
            {39, 39},
            {41, 49},
            {51, 51},
            {53, 53},
            {55, 55},
    };
    auto vec = window->get_pending_seq();
    EXPECT_EQ(window->get_expected_size(), 27);
    EXPECT_EQ(vec.size(), 7);
    int i = 0;
    for (const auto &item: vec) {
        EXPECT_EQ(v[i].first, item.first);
        EXPECT_EQ(v[i].second, item.second);
        ++i;
    }


    window->clear();
    window->set_window_size(20);
    window->set_max_sequence(50);
    window->set_initial_sequence(45);
    window->set_max_delay(0);

    window->arrived_packet(46, 0, -1);
    window->arrived_packet(48, 0, -1);
    window->arrived_packet(2, 0, -1);
    window->arrived_packet(4, 0, -1);
    window->arrived_packet(5, 0, -1);
    window->arrived_packet(6, 0, -1);
    window->arrived_packet(10, 0, -1);
    std::vector<std::pair<uint32_t, uint32_t>> vv{
            {45, 45},
            {47, 47},
            {49, 1},
            {3, 3},
            {7, 9},
    };
    auto vecc = window->get_pending_seq();
    EXPECT_EQ(16, window->get_expected_size());
    EXPECT_EQ(5, vecc.size());
    i = 0;
    for (const auto &item: vecc) {
        EXPECT_EQ(vv[i].first, item.first);
        EXPECT_EQ(vv[i].second, item.second);
        ++i;
    }
}

TEST(sliding_window, sliding_window) {
    logger::initialize("logs/test_sliding_window.log", spdlog::level::trace);
    auto window = std::make_shared<basic_window>();

    window->set_window_size(6);
    window->set_max_sequence(13);
    window->set_max_delay(0);
    window->set_initial_sequence(2);

    bool is_running = true;
    static std::atomic<bool> _is_run{false};

    push_operation(window);
    find_operation(window);
    sequence_to(window);
    receiver_queue(window);
}