
/*
  Copyright(c) 2015 - 2022 Denis Blank <denis.blank at outlook dot com>

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <test-continuable.hpp>

using namespace cti;

static int const CANARY = 19372;

TYPED_TEST(single_dimension_tests, operations_async) {
  ASSERT_ASYNC_COMPLETION(async([] {
    //
  }));

  ASSERT_ASYNC_RESULT(async([] {
                        //
                        return CANARY;
                      }),
                      CANARY);

  ASSERT_ASYNC_RESULT(async([] {
                        //
                        return std::make_tuple(CANARY, 2, CANARY);
                      }),
                      CANARY, 2, CANARY);
}

TYPED_TEST(single_dimension_tests, operations_async_on_dropping) {
  bool invoked = false;
  auto executor = [&](auto&& work) {
    EXPECT_FALSE(invoked);
    invoked = true;
    (void)work;
  };

  ASSERT_ASYNC_INCOMPLETION(async_on(
      [] {
        FAIL(); //
      },
      executor));

  ASSERT_TRUE(invoked);
}

TYPED_TEST(single_dimension_tests, operations_async_on_executor) {
  bool invoked = false;
  auto executor = [&](auto&& work) {
    // We can move the worker object
    auto local = std::forward<decltype(work)>(work);
    EXPECT_FALSE(invoked);
    // We can invoke the worker object
    std::move(local)();
    EXPECT_TRUE(invoked);
  };

  ASSERT_ASYNC_COMPLETION(async_on(
      [&invoked] {
        EXPECT_FALSE(invoked);
        invoked = true;
      },
      executor));

  ASSERT_TRUE(invoked);
}
