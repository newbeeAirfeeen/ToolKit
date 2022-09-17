// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_URI_ADVANCE_INC
#define NETWORK_DETAIL_URI_ADVANCE_INC

#include "Util/string_view.h"
#include "uri_parts.hpp"
namespace net {
    namespace detail {
        uri_part copy_part(const std::string &part, string_view::const_iterator &it);

        void advance_parts(string_view uri_view, uri_parts &parts,
                           const uri_parts &existing_parts);
    }// namespace detail
}// namespace net

#endif// NETWORK_DETAIL_URI_ADVANCE_INC
