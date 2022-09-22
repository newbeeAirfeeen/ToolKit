//
// Created by 沈昊 on 2022/8/1.
//

#ifndef TOOLKIT_RANDOM_HPP
#define TOOLKIT_RANDOM_HPP

#include <numeric>
#include <random>
#include <string>
#include <type_traits>
std::string makeRandStr(int sz, bool printable);

std::mt19937 &get_mt();
template<typename T>
T rng_integer(T range_begin = (std::numeric_limits<T>::min)(), T range_end = (std::numeric_limits<T>::max)()) {
    std::uniform_int_distribution<T> dis(range_begin, range_end);
    return dis(get_mt());
}

//template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
//auto get_random() -> typename std::remove_reference<typename std::remove_cv<T>::type>::type {
//    using return_type = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
//    static thread_local std::random_device ran;
//    static thread_local std::mt19937 sgen(ran());
//    std::uniform_real_distribution<>
//}

#endif//TOOLKIT_RANDOM_HPP
