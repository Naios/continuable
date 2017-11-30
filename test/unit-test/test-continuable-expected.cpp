
/*
  Copyright(c) 2015 - 2017 Denis Blank <denis.blank at outlook dot com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <memory>
#include <utility>

#include <continuable/detail/expected.hpp>
#include <continuable/detail/types.hpp>

#include "test-continuable.hpp"

using cti::detail::types::error_type;
using cti::detail::util::expected;

static int const CANARY = 373671;

template <typename T>
struct expected_all_tests : testing::Test {
  template <typename V>
  auto supply(V&& value) const {
    return std::forward<V>(value);
  }
  template <typename V>
  auto get(V& value) const {
    return value;
  }
};
template <typename T>
struct expected_all_tests<expected<std::unique_ptr<T>>> : testing::Test {
  template <typename V>
  auto supply(V&& value) const {
    return std::make_unique<T>(std::forward<V>(value));
  }
  template <typename V>
  auto get(std::unique_ptr<V>& value) const {
    return *value;
  }
};

using copyable_type = expected<int>;
using unique_type = expected<std::unique_ptr<int>>;

using expected_test_types = testing::Types<unique_type, copyable_type>;

TYPED_TEST_CASE(expected_all_tests, expected_test_types);

TYPED_TEST(expected_all_tests, can_carry_errors) {
  {
    TypeParam e(this->supply(CANARY));

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(this->get(*e), CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    TypeParam e(error_type{});

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TYPED_TEST(expected_all_tests, is_empty_constructible) {
  TypeParam e;
  (void)e;
}

TYPED_TEST(expected_all_tests, is_move_constructible) {
  {
    TypeParam e(TypeParam(this->supply(CANARY)));

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(this->get(*e), CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    TypeParam e(TypeParam(error_type{}));
    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TYPED_TEST(expected_all_tests, is_move_assignable) {
  {
    TypeParam old(this->supply(CANARY));
    TypeParam e;
    e = std::move(old);

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(this->get(*e), CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    TypeParam old(error_type{});
    TypeParam e;
    e = std::move(old);

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TEST(expected_copyable_tests, is_copy_constructible) {
  {
    copyable_type const e_old(CANARY);
    copyable_type e(e_old);

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(*e, CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    copyable_type const e_old(error_type{});
    copyable_type e(e_old);

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TEST(expected_copyable_tests, is_copy_assignable) {
  {
    copyable_type const e_old(CANARY);
    copyable_type e;
    e = e_old;

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(*e, CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    copyable_type const e_old(error_type{});
    copyable_type e;
    e = e_old;

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}
