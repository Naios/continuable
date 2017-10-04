
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

#include "test-continuable.hpp"

using namespace cti;
using namespace cti::detail;

TYPED_TEST(single_dimension_tests, are_partial_callable) {
  EXPECT_ASYNC_RESULT(this->supply(1, 2).then([] {
    // ...
  }));

  EXPECT_ASYNC_RESULT(this->supply(0xDF, 0xDD, 3, 4).then([](int a, int b) {
    EXPECT_EQ(a, 0xDF);
    EXPECT_EQ(b, 0xDD);
  }));
}

TYPED_TEST(single_dimension_tests, are_dispatchable) {
  bool invoked = false;
  auto executor = [&](auto&& work) {
    // We can move the worker object
    auto local = std::forward<decltype(work)>(work);
    ASSERT_FALSE(invoked);
    // We can invoke the worker object
    std::move(local)();
    ASSERT_TRUE(invoked);
  };

  auto chain = this->supply().then(
      [&] {
        ASSERT_FALSE(invoked);
        invoked = true;
      },
      executor);

  ASSERT_ASYNC_COMPLETION(std::move(chain));
}
