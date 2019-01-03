
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
#include <test-continuable.hpp>

using namespace cti;

struct mypromise {
  void operator()() && {
  }

  void operator()(exception_arg_t, exception_t) && {
  }
};

struct my_continuation {
  template <typename Callback>
  void operator()(Callback&&) {
  }

  bool operator()(is_ready_arg_t) const noexcept {
    return true;
  }

  std::tuple<> operator()(query_arg_t) {
    return std::make_tuple();
  }
};

TEST(single_erasure_test, is_assignable_from_promise) {
  detail::erasure::callback<> p(mypromise{});
  std::move(p)();
}

TEST(single_erasure_test, is_assignable_from_continuation) {
  detail::erasure::continuation<> c(my_continuation{});
  ASSERT_TRUE(c(is_ready_arg_t{}));
  c(mypromise{});
}

TEST(single_erasure_test, is_constructible_from_work) {
  bool flag = false;

  work mywork([&] {
    EXPECT_FALSE(flag);
    flag = true;
  });

  ASSERT_FALSE(flag);
  mywork();
  ASSERT_TRUE(flag);
}

TEST(single_erasure_test, is_assignable_from_work) {
  bool flag = false;

  work mywork;

  mywork = [&] {
    EXPECT_FALSE(flag);
    flag = true;
  };

  ASSERT_FALSE(flag);
  mywork();
  ASSERT_TRUE(flag);
}
