//
// Created by 沈昊 on 2022/9/20.
//
#include "string_util.hpp"
namespace string_util {
    using index_type = typename string_view::size_type;
    auto split(string_view target, const string_view &split_) -> std::list<string_view> {
        if (split_.empty()) return {};
        auto size = target.size();
        auto begin = target.data();
        std::list<string_view> vec;
        while (!target.empty()) {
            index_type end = target.find(split_);
            if (end == string_view::npos) {
                vec.emplace_back(target.data(), size - (target.data() - begin));
                break;
            }
            if (end == 0) {
                target.remove_prefix(1);
                continue;
            }
            vec.emplace_back(string_view{target.data(), end});
            target.remove_prefix(end);
            target.remove_prefix(split_.size());
        }
        return vec;
    }
};// namespace string_util