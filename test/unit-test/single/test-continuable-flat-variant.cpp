
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

#include <continuable/detail/flat-variant.hpp>

#include <test-continuable.hpp>

using cti::detail::container::flat_variant;

static int const CANARY = 373671;

using int_variant = flat_variant<int, tag1, tag2>;

TEST(flat_variant_single_test, is_move_constructible) {
  {
    int_variant e_old(CANARY);
    int_variant e(std::move(e_old));

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<int>());
    EXPECT_EQ(e.cast<int>(), CANARY);
  }

  {
    int_variant e(int_variant(tag1{}));

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<tag1>());
  }
}

TEST(flat_variant_single_test, is_value_move_assignable) {
  int_variant old(CANARY);
  int_variant e;
  e = std::move(old);

  EXPECT_TRUE(bool(e));
  EXPECT_TRUE(e.is<int>());
  EXPECT_EQ(e.cast<int>(), CANARY);
}

TEST(flat_variant_single_test, is_copy_constructible) {
  {
    int_variant const e_old(CANARY);
    int_variant e(e_old);

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<int>());
    EXPECT_EQ(e.cast<int>(), CANARY);
  }

  {
    int_variant const e_old(tag1{});
    int_variant e(e_old);

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<tag1>());
  }
}

TEST(flat_variant_single_test, is_copy_assignable) {
  {
    int_variant const e_old(CANARY);
    int_variant e;
    e = e_old;

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<int>());
    EXPECT_EQ(e.cast<int>(), CANARY);
  }

  {
    int_variant const e_old(tag1{});
    int_variant e;
    e = e_old;

    EXPECT_TRUE(bool(e));
    EXPECT_TRUE(e.is<tag1>());
  }
}

// This regression test shows a memory leak which happens when using the
// expected class move constructed from another expected object.
TEST(flat_variant_single_test, test_leak_regression) {
  // expected_all_tests<cti::detail::util::expected<std::__1::unique_ptr<int,
  //        std::__1::default_delete<int> > > >::supply<int const&>(int const&)
  //        const
  //        continuable/build/../test/unit-test/test-continuable-expected.cpp:52
  // 3:     #3 0x11cf07a in
  //        expected_all_tests_is_value_assignable_Test<cti::detail::util::expected<std::__1::unique_ptr<int,
  //        std::__1::default_delete<int> > > >::TestBody()
  //        continuable/build/../test/unit-test/test-continuable-expected.cpp:133:15
  // 3:     #4 0x1339e4e in void
  //        testing::internal::HandleSehExceptionsInMethodIfSupported<testing::Test,
  //        void>(testing::Test*, void (testing::Test::*)(), char const*)
  //        continuable/build/../dep/googletest/googletest/googletest/src/gtest.cc:2395:10
  using type = std::shared_ptr<int>;

  auto const validate = [](auto&& variant, long use) {
    ASSERT_TRUE(variant.template is<type>());
    ASSERT_EQ((*variant.template cast<type>()), CANARY);
    ASSERT_EQ(variant.template cast<type>().use_count(), use);
  };

  bool destroyed = false;
  {
    type ptr(new int(CANARY), [&](int* val) {
      destroyed = true;
      delete val;
    });

    auto e1(flat_variant<type>(std::move(ptr)));
    validate(e1, 1);

    auto e2 = std::move(e1);
    validate(e2, 1);

    flat_variant<type> e3;
    e3 = std::move(e2);
    validate(e3, 1);

    {
      flat_variant<type> ec(e3);
      validate(ec, 2);
    }

    {
      flat_variant<type> ec = e3;
      validate(ec, 2);
    }

    {
      flat_variant<type> ec;
      ec = e3;
      validate(ec, 2);
    }
  }

  ASSERT_TRUE(destroyed);
}
