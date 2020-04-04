
/*
  Copyright(c) 2015 - 2020 Denis Blank <denis.blank at outlook dot com>

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

TEST(promise_tests, are_invalidated) {
  ASSERT_ASYNC_COMPLETION(make_continuable<void>([](promise<> promise) {
    EXPECT_TRUE(promise);
    promise.set_value();
    EXPECT_FALSE(promise);
  }));
}

TEST(promise_tests, are_move_assignable) {
  ASSERT_ASYNC_COMPLETION(make_continuable<void>([](auto&& initial) {
    promise<> other;
    EXPECT_FALSE(other);
    EXPECT_TRUE(initial);

    other = std::forward<decltype(initial)>(initial);
    EXPECT_TRUE(other);
    other.set_value();
    EXPECT_FALSE(other);
  }));
}
