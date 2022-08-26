//
// Created by 沈昊 on 2022/8/26.
//

#ifndef TOOLKIT_HOOK_HPP
#define TOOLKIT_HOOK_HPP
#include <type_traits>
/// 添加对应实现，gcc 4.8.5 中没有is_trivially_copy_constructable 的问题，导致编译通不过
// template<class T>
// struct is_trivially_copy_constructible :
//     std::is_trivially_constructible<T, typename std::add_lvalue_reference<
//                                            typename
//                                            std::add_const<T>::type>::type>
//{};

template <typename T>
struct is_trivial_copy_
    : public std::integral_constant<bool,
                                    std::is_trivial<T>::value &&
                                        std::is_copy_constructible<T>::value> {
};

template <typename A, typename B>
struct AND : public std::conditional<A::value, B, A>::type {};

template <typename T, typename... Args>
struct is_trivial_constructable
    : public AND<std::is_constructible<T>,
                 std::integral_constant<bool, is_trivial_copy_<T>::value>> {};

template <typename T>
struct is_trivially_copy_constructable
    : public is_trivial_constructable<
          T, typename std::add_lvalue_reference<
                 typename std::add_const<T>::type>::type> {};
#endif  // TOOLKIT_HOOK_HPP
