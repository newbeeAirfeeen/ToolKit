//
// Created by 沈昊 on 2022/9/20.
//

#ifndef TOOLKIT_STRING_UTIL_HPP
#define TOOLKIT_STRING_UTIL_HPP
#include "string_view.h"
#include <list>
namespace string_util {
    std::list<string_view> split(string_view target, const string_view &split_);
};
#endif//TOOLKIT_STRING_UTIL_HPP
