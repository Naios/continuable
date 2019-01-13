
/*
  Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>

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

static int const CANARY = 39472;

TYPED_TEST(single_dimension_tests, operations_loop_completion) {
  ASSERT_ASYNC_COMPLETION(loop([] {
    //
    return make_ready_continuable(make_result());
  }));

  ASSERT_ASYNC_RESULT(loop(
                          [](auto&&... args) {
                            //
                            return make_ready_continuable(make_result(
                                std::forward<decltype(args)>(args)...));
                          },
                          CANARY),
                      CANARY);

  ASSERT_ASYNC_RESULT(loop(
                          [](auto&&... args) {
                            //
                            return make_ready_continuable(make_result(
                                std::forward<decltype(args)>(args)...));
                          },
                          CANARY, 2, CANARY),
                      CANARY, 2, CANARY);
}

TYPED_TEST(single_dimension_tests, operations_loop_looping) {
  int i = 0;

  ASSERT_ASYNC_COMPLETION(range_loop(
      [&](int current) {
        EXPECT_EQ(current, i);
        ++i;
        return make_ready_continuable();
      },
      0, 10));

  ASSERT_EQ(i, 10);
}
