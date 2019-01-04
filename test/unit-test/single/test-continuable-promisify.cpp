
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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <test-continuable.hpp>

template <typename T, typename Callback>
void async_supply(T&& value, Callback&& callback) {
  std::forward<Callback>(callback)(cti::exception_t{}, std::forward<T>(value));
}

TEST(promisify_tests, promisify_from) {

  auto c = cti::promisify<int>::from(
      [&](auto&&... args) {
        async_supply(std::forward<decltype(args)>(args)...);
      },
      36354);

  EXPECT_ASYNC_RESULT(std::move(c), 36354);
}

TEST(promisify_tests, promisify_from_lvalue) {
  int value = 0;
  auto c = cti::promisify<int>::from(
      [&](auto&&... args) {
        async_supply(std::forward<decltype(args)>(args)...);
      },
      value);

  value = 36354;
  EXPECT_ASYNC_RESULT(std::move(c), 36354);
}

TEST(promisify_tests, promisify_with) {
  int value = 0;
  auto c = cti::promisify<int>::with(
      [](auto&& promise, auto&& /*e*/, int const& value) {
        EXPECT_EQ(value, 36354);
        promise.set_exception(cti::exception_t{});
      },
      [&](auto&&... args) {
        async_supply(std::forward<decltype(args)>(args)...);
      },
      value);

  value = 36354;
  ASSERT_ASYNC_EXCEPTION_COMPLETION(std::move(c));
}
