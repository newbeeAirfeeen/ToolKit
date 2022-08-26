//
// Created by 沈昊 on 2022/6/30.
//
#include <future>
#include <iostream>
#include <memory>
#include <utility>
#include <spdlog/logger.hpp>
template<typename In, typename Out>
class multiple_future {
public:
    using in_type = typename std::remove_cv<typename std::remove_reference<In>::type>::type;
    using out_type = typename std::remove_cv<typename std::remove_reference<Out>::type>::type;

    void set(const in_type &in, const std::weak_ptr<std::future<out_type>> &out) {
    }

private:
    std::pair<in_type, std::weak_ptr<std::future<out_type>>> _pair;
};
#include <net/buffer.hpp>
#include <spdlog/logger.hpp>
#include <fmt/fmt.h>
#include <thread>
#include <vector>
struct A {
    A() {
        Info("init");
    }
};
void print() {
    static thread_local std::shared_ptr<A> buff = std::make_shared<A>();
    using namespace std;
    cout << std::hex << std::showbase << (void*)&buff << endl;
}

int main() {
    logger::initialize("logs/test_logger.log", spdlog::level::trace);

    std::vector<std::thread> vec;
    vec.reserve(10);
    for (int i = 0; i < 10; i++) {
        vec.emplace_back([]() {
            print();
        });
    }
    std::this_thread::sleep_for(std::chrono::minutes(60));
    return 0;
}