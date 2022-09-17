// Copyright (c) Glyn Matthews 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <string>
#include "Util/optional.hpp"
#include "Util/string_view.h"

TEST(optional_test, empty_optional) {
  optional<int> opt;
  ASSERT_FALSE(opt);
}

TEST(optional_test, empty_optional_constructed_with_nullopt) {
  optional<int> opt{nullopt};
  ASSERT_FALSE(opt);
}

TEST(optional_test, empty_optional_string) {
  optional<std::string> opt{};
  ASSERT_FALSE(opt);
}

TEST(optional_test, empty_optional_string_with_nullopt) {
  optional<std::string> opt{nullopt};
  ASSERT_FALSE(opt);
}

TEST(optional_test, value_constructor) {
  optional<int> opt{42};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, value_constructor_string) {
  optional<std::string> opt{"banana"};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, rvalue_ref_constructor) {
  int value = 42;
  optional<int> opt{std::move(value)};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, rvalue_ref_constructor_string) {
  std::string value = "banana";
  optional<std::string> opt{std::move(value)};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, nullopt_copy_constructor) {
  optional<int> other{nullopt};
  optional<int> opt{other};
  ASSERT_FALSE(opt);
}

TEST(optional_test, nullopt_move_constructor) {
  optional<int> other{nullopt};
  optional<int> opt{std::move(other)};
  ASSERT_FALSE(opt);
}

TEST(optional_test, value_copy_constructor) {
  optional<int> other{42};
  optional<int> opt{other};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, value_move_constructor) {
  optional<int> other{42};
  optional<int> opt{std::move(other)};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, value_copy_constructor_string) {
  optional<std::string> other{"banana"};
  optional<std::string> opt{other};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, value_move_constructor_string) {
  optional<std::string> other{"banana"};
  optional<std::string> opt{std::move(other)};
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, nullopt_assignment) {
  optional<int> opt(42);
  opt = nullopt;
  ASSERT_FALSE(opt);
}

TEST(optional_test, nullopt_assignment_string) {
  optional<std::string> opt("banana");
  opt = nullopt;
  ASSERT_FALSE(opt);
}

TEST(optional_test, value_copy_assigment) {
  optional<int> opt{};
  optional<int> other{42};
  opt = other;
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, value_move_assignment) {
  optional<int> opt{};
  optional<int> other{42};
  opt = std::move(other);
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, 42);
}

TEST(optional_test, value_copy_assignment_string) {
  optional<std::string> opt{};
  optional<std::string> other{"banana"};
  opt = other;
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, value_move_assignment_string) {
  optional<std::string> opt{};
  optional<std::string> other{"banana"};
  opt = std::move(other);
  ASSERT_TRUE(opt);
  ASSERT_EQ(*opt, "banana");
}

TEST(optional_test, value_or_reference) {
  optional<std::string> opt;
  auto result = opt.value_or("other");
  ASSERT_EQ("other", result);
}

TEST(optional_test, value_or_reference_with_value) {
  optional<std::string> opt("this");
  auto result = opt.value_or("other");
  ASSERT_EQ("this", result);
}

TEST(optional_test, value_or_rvalue_reference) {
  std::string other("other");
  auto result = optional<std::string>().value_or(other);
  ASSERT_EQ("other", result);
}

TEST(optional_test, value_or_rvalue_reference_with_value) {
  std::string other("other");
  auto result = optional<std::string>("this").value_or(other);
  ASSERT_EQ("this", result);
}

TEST(optional_test, assign_nullopt_to_nullopt) {
	optional<std::string> opt;
	opt = nullopt;
}