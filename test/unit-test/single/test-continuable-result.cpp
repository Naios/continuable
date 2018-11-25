
/*
  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

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
#include <continuable/continuable-result.hpp>
#include <continuable/detail/core/types.hpp>
#include <test-continuable.hpp>

using cti::exception_t;
using cti::make_result;
using cti::result;

static int const CANARY = 373671;

template <typename T>
struct result_all_tests : testing::Test {
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
struct result_all_tests<result<std::unique_ptr<T>>> : testing::Test {
  template <typename V>
  auto supply(V&& value) const {
    return std::make_unique<T>(std::forward<V>(value));
  }
  template <typename V>
  auto get(std::unique_ptr<V>& value) const {
    return *value;
  }
};

using copyable_type = result<int>;
using unique_type = result<std::unique_ptr<int>>;

using result_test_types = testing::Types<unique_type, copyable_type>;

TYPED_TEST_CASE(result_all_tests, result_test_types);

TYPED_TEST(result_all_tests, is_default_constructible) {
  TypeParam e;
  result<> e1;
  static_assert(std::is_void<decltype(e1.get_value())>::value,
                "get_value() must return void here!");
  EXPECT_TRUE(e1.is_empty());

  result<int> e2;
  EXPECT_TRUE(e2.is_empty());

  result<int, int> e3;
  EXPECT_TRUE(e3.is_empty());

  auto empty = make_result();
  EXPECT_TRUE(empty.is_empty());
}

TYPED_TEST(result_all_tests, can_carry_errors) {
  {
    TypeParam e(this->supply(CANARY));

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(this->get(*e), CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    TypeParam e(supply_test_exception());

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TYPED_TEST(result_all_tests, is_empty_constructible) {
  TypeParam e;
  (void)e;
}

TYPED_TEST(result_all_tests, is_move_constructible) {
  {
    TypeParam e(TypeParam(this->supply(CANARY)));

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(this->get(*e), CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    TypeParam e{TypeParam{supply_test_exception()}};
    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TYPED_TEST(result_all_tests, is_value_move_assignable) {
  TypeParam old(this->supply(CANARY));
  TypeParam e;
  e = std::move(old);

  EXPECT_TRUE(bool(e));
  EXPECT_EQ(this->get(*e), CANARY);
  EXPECT_TRUE(e.is_value());
  EXPECT_FALSE(e.is_exception());
}

TYPED_TEST(result_all_tests, is_error_move_assignable) {
  TypeParam old(supply_test_exception());
  TypeParam e;
  e = std::move(old);

  EXPECT_FALSE(bool(e));
  EXPECT_FALSE(e.is_value());
  EXPECT_TRUE(e.is_exception());
}

TEST(result_copyable_tests, is_copy_constructible) {
  {
    copyable_type const e_old(CANARY);
    copyable_type e(e_old);

    EXPECT_TRUE(bool(e));
    EXPECT_EQ(*e, CANARY);
    EXPECT_TRUE(e.is_value());
    EXPECT_FALSE(e.is_exception());
  }

  {
    copyable_type const e_old(supply_test_exception());
    copyable_type e(e_old);

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TEST(result_copyable_tests, is_copy_assignable) {
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
    copyable_type const e_old(supply_test_exception());
    copyable_type e;
    e = e_old;

    EXPECT_FALSE(bool(e));
    EXPECT_FALSE(e.is_value());
    EXPECT_TRUE(e.is_exception());
  }
}

TYPED_TEST(result_all_tests, is_constructible_from_error_helper) {
  cti::exceptional_result e1(supply_test_exception());
  {
    TypeParam e2 = e1;
    EXPECT_FALSE(bool(e2));
    EXPECT_FALSE(e2.is_value());
    EXPECT_TRUE(e2.is_exception());
  }
  auto e2 = std::move(e1);

  TypeParam e(std::move(e2));

  EXPECT_FALSE(bool(e));
  EXPECT_FALSE(e.is_value());
  EXPECT_TRUE(e.is_exception());
}

TYPED_TEST(result_all_tests, is_assignable_from_error_helper) {
  cti::exceptional_result e1(supply_test_exception());
  {
    TypeParam e2 = e1;
    EXPECT_FALSE(bool(e2));
    EXPECT_FALSE(e2.is_value());
    EXPECT_TRUE(e2.is_exception());
  }
  auto e2 = std::move(e1);

  TypeParam e;
  e = std::move(e2);

  EXPECT_FALSE(bool(e));
  EXPECT_FALSE(e.is_value());
  EXPECT_TRUE(e.is_exception());
}

TYPED_TEST(result_all_tests, is_constructible_from_empty_helper) {
  cti::empty_result e1;
  {
    auto e2 = e1;
    (void)e2;
  }
  auto e2 = std::move(e1);

  TypeParam e(std::move(e2));

  EXPECT_FALSE(bool(e));
  EXPECT_FALSE(e.is_value());
  EXPECT_TRUE(e.is_empty());
}

TYPED_TEST(result_all_tests, is_assignable_from_empty_helper) {
  cti::empty_result e1;
  {
    auto e2 = e1;
    (void)e2;
  }
  auto e2 = std::move(e1);

  TypeParam e;
  e = std::move(e2);

  EXPECT_FALSE(bool(e));
  EXPECT_FALSE(e.is_value());
  EXPECT_TRUE(e.is_empty());
}

// This regression test shows a memory leak which happens when using the
// result class move constructed from another result object.
TEST(result_single_test, test_leak_regression) {
  // result_all_tests<cti::detail::util::result<std::__1::unique_ptr<int,
  //        std::__1::default_delete<int> > > >::supply<int const&>(int const&)
  //        const
  //        continuable/build/../test/unit-test/test-continuable-result.cpp:52
  // 3:     #3 0x11cf07a in
  //        result_all_tests_is_value_assignable_Test<cti::detail::util::result<std::__1::unique_ptr<int,
  //        std::__1::default_delete<int> > > >::TestBody()
  //        continuable/build/../test/unit-test/test-continuable-result.cpp:133:15
  // 3:     #4 0x1339e4e in void
  //        testing::internal::HandleSehExceptionsInMethodIfSupported<testing::Test,
  //        void>(testing::Test*, void (testing::Test::*)(), char const*)
  //        continuable/build/../dep/googletest/googletest/googletest/src/gtest.cc:2395:10

  bool destroyed = false;
  {
    std::shared_ptr<int> ptr(new int(0), [&](int* val) {
      destroyed = true;
      delete val;
    });

    auto e(result<std::shared_ptr<int>>(std::move(ptr)));
    ASSERT_TRUE(e.is_value());
  }

  ASSERT_TRUE(destroyed);
}
