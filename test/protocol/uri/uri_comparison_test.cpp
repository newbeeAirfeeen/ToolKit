// Copyright (c) Glyn Matthews 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include "protocol/uri/uri.hpp"


TEST(uri_comparison_test, equality_test) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs("http://www.example.com/");
  ASSERT_EQ(lhs, rhs);
}

TEST(uri_comparison_test, equality_test_capitalized_scheme) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs("HTTP://www.example.com/");
  ASSERT_NE(lhs.compare(rhs, net::uri_comparison_level::string_comparison), 0);
}

TEST(uri_comparison_test, equality_test_capitalized_scheme_with_case_normalization) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs("HTTP://www.example.com/");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, DISABLED_equality_test_capitalized_host) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs("http://WWW.EXAMPLE.COM/");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_test_with_single_dot_segment) {
  net::uri lhs("http://www.example.com/./path");
  net::uri rhs("http://www.example.com/path");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_test_with_double_dot_segment) {
  net::uri lhs("http://www.example.com/1/../2/");
  net::uri rhs("http://www.example.com/2/");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, DISABLED_given_example_test) {
  net::uri lhs("example://a/b/c/%7Bfoo%7D");
  net::uri rhs("eXAMPLE://a/./b/../b/%63/%7bfoo%7d");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}

TEST(uri_comparison_test, equality_empty_lhs) {
  net::uri lhs;
  net::uri rhs("http://www.example.com/");
  ASSERT_NE(lhs, rhs);
}

TEST(uri_comparison_test, equality_empty_rhs) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs;
  ASSERT_NE(lhs, rhs);
}

TEST(uri_comparison_test, inequality_test) {
  net::uri lhs("http://www.example.com/");
  net::uri rhs("http://www.example.com/");
  ASSERT_FALSE(lhs != rhs);
}

TEST(uri_comparison_test, less_than_test) {
  // lhs is lexicographically less than rhs
  net::uri lhs("http://www.example.com/");
  net::uri rhs("http://www.example.org/");
  ASSERT_LT(lhs, rhs);
}

TEST(uri_comparison_test, percent_encoded_query_reserved_chars) {
  net::uri lhs("http://www.example.com?foo=%5cbar");
  net::uri rhs("http://www.example.com?foo=%5Cbar");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}


TEST(uri_comparison_test, percent_encoded_query_unreserved_chars) {
  net::uri lhs("http://www.example.com?foo=%61%6c%70%68%61%31%32%33%2d%2e%5f%7e");
  net::uri rhs("http://www.example.com?foo=alpha123-._~");
  ASSERT_EQ(lhs.compare(rhs, net::uri_comparison_level::syntax_based), 0);
}
