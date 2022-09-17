// Copyright (c) Glyn Matthews 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "uri_builder.hpp"
#include "algorithm.hpp"
#include "uri_normalize.hpp"
#include "uri_parse_authority.hpp"
#include <locale>

namespace net {
    uri_builder::uri_builder(const net::uri &base_uri) {
        if (base_uri.has_scheme()) {
            scheme_ = base_uri.scheme().to_string();
        }

        if (base_uri.has_user_info()) {
            user_info_ = base_uri.user_info().to_string();
        }

        if (base_uri.has_host()) {
            host_ = base_uri.host().to_string();
        }

        if (base_uri.has_port()) {
            port_ = base_uri.port().to_string();
        }

        if (base_uri.has_path()) {
            path_ = base_uri.path().to_string();
        }

        if (base_uri.has_query()) {
            query_ = base_uri.query().to_string();
        }

        if (base_uri.has_fragment()) {
            fragment_ = base_uri.fragment().to_string();
        }
    }

    uri_builder::~uri_builder() noexcept {}

    net::uri uri_builder::uri() const { return net::uri(*this); }

    void uri_builder::set_scheme(string_type &&scheme) {
        // validate scheme is valid and normalize
        scheme_ = scheme;
        detail::transform(*scheme_, std::begin(*scheme_),
                          [](char ch) { return std::tolower(ch, std::locale()); });
    }

    void uri_builder::set_user_info(string_type &&user_info) {
        user_info_ = string_type();
        net::uri::encode_user_info(std::begin(user_info), std::end(user_info),
                                       std::back_inserter(*user_info_));
    }

    uri_builder &uri_builder::clear_user_info() {
        user_info_ = nullopt;
        return *this;
    }

    void uri_builder::set_host(string_type &&host) {
        host_ = string_type();
        net::uri::encode_host(std::begin(host), std::end(host),
                                  std::back_inserter(*host_));
        detail::transform(*host_, std::begin(*host_),
                          [](char ch) { return std::tolower(ch, std::locale()); });
    }

    void uri_builder::set_port(string_type &&port) {
        port_ = string_type();
        net::uri::encode_port(std::begin(port), std::end(port),
                                  std::back_inserter(*port_));
    }

    uri_builder &uri_builder::clear_port() {
        port_ = nullopt;
        return *this;
    }

    void uri_builder::set_authority(string_type &&authority) {
        optional<detail::uri_part> user_info, host, port;
        string_view view(authority);
        uri::const_iterator it = std::begin(view), last = std::end(view);
        detail::parse_authority(it, last, user_info, host, port);

        if (user_info) {
            set_user_info(user_info->to_string());
        }

        if (host) {
            set_host(host->to_string());
        }

        if (port) {
            set_port(port->to_string());
        }
    }

    void uri_builder::set_path(string_type &&path) {
        path_ = string_type();
        net::uri::encode_path(std::begin(path), std::end(path),
                                  std::back_inserter(*path_));
    }

    uri_builder &uri_builder::clear_path() {
        path_ = nullopt;
        return *this;
    }

    void uri_builder::append_query_component(string_type &&name) {
        if (!query_) {
            query_ = string_type();
        } else {
            query_->append("&");
        }
        net::uri::encode_query_component(std::begin(name), std::end(name),
                                             std::back_inserter(*query_));
    }

    void uri_builder::append_query_key_value_pair(string_type &&key,
                                                  string_type &&value) {
        if (!query_) {
            query_ = string_type();
        } else {
            query_->push_back('&');
        }
        net::uri::encode_query_key_value_pair(std::begin(key), std::end(key),
                                                  std::begin(value), std::end(value),
                                                  std::back_inserter(*query_));
    }

    uri_builder &uri_builder::clear_query() {
        query_ = nullopt;
        return *this;
    }

    void uri_builder::set_fragment(string_type &&fragment) {
        fragment_ = string_type();
        net::uri::encode_fragment(std::begin(fragment), std::end(fragment),
                                      std::back_inserter(*fragment_));
    }

    uri_builder &uri_builder::clear_fragment() {
        fragment_ = nullopt;
        return *this;
    }
}// namespace net
