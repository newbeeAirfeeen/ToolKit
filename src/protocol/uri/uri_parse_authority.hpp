// Copyright 2013-2016 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_DETAIL_URI_PARSE_AUTHORITY_INC
#define NETWORK_DETAIL_URI_PARSE_AUTHORITY_INC

#include "Util/optional.hpp"
#include "Util/string_view.h"
#include "uri_parts.hpp"

namespace net {
    namespace detail {
        bool parse_authority(string_view::const_iterator &first,
                             string_view::const_iterator last,
                             optional<uri_part> &user_info, optional<uri_part> &host,
                             optional<uri_part> &port);
    }// namespace detail
}// namespace net

#endif// NETWORK_DETAIL_URI_PARSE_AUTHORITY_INC
