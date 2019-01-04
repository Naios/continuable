
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

#include <continuable/detail/features.hpp>
#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS
#include <exception>
#endif // CONTINUABLE_WITH_NO_EXCEPTIONS

#include <tuple>

/// Resolves the given promise asynchonously
template <typename S>
cti::continuable<> resolve_async(S&& supplier) {
  // 0 args
  co_await supplier();

  // 1 args
  int a1 = co_await supplier(1);
  EXPECT_EQ(a1, 1);

  // 2-n args
  std::tuple<int, int> a2 = co_await supplier(1, 2);
  EXPECT_EQ(a2, std::make_tuple(1, 2));

  co_return;
}

template <typename S>
cti::continuable<int> resolve_async_one(S&& supplier) {
  // Pseudo wait
  co_await supplier();

  co_return 4644;
}

template <typename S>
cti::continuable<int, int, int, int> resolve_async_multiple(S&& supplier) {
  // Pseudo wait
  co_await supplier();

  co_return std::make_tuple(0, 1, 2, 3);
}

TYPED_TEST(single_dimension_tests, are_awaitable) {
  auto const supply = [&](auto&&... args) {
    // Supplies the current tested continuable
    return this->supply(std::forward<decltype(args)>(args)...);
  };

  EXPECT_ASYNC_RESULT(resolve_async(supply));
  EXPECT_ASYNC_RESULT(resolve_async_one(supply), 4644);
  EXPECT_ASYNC_RESULT(resolve_async_multiple(supply), 0, 1, 2, 3);
}

#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS

struct await_exception : std::exception {
  char const* what() const noexcept override {
    return "await_exception";
  }

  bool operator==(await_exception const&) const noexcept {
    return true;
  }
};

/// Resolves the given promise asynchonously through an exception
template <typename S>
cti::continuable<> resolve_async_exceptional(S&& supplier) {
  // 0 args
  co_await supplier();

  // 1 args
  int a1 = co_await supplier(1);
  EXPECT_EQ(a1, 1);

  // 2-n args
  std::tuple<int, int> a2 = co_await supplier(1, 2);
  EXPECT_EQ(a2, std::make_tuple(1, 2));

  // GTest ASSERT_THROW isn't co_await friendly yet:
  // clang: 'return statement not allowed in coroutine; did you mean
  //        'co_return'?'
  EXPECT_THROW(co_await supplier().then([] { throw await_exception{}; }),
               await_exception);

  co_return;
}

TYPED_TEST(single_dimension_tests, are_awaitable_with_exceptions) {
  auto const& supply = [&](auto&&... args) {
    // Supplies the current tested continuable
    return this->supply(std::forward<decltype(args)>(args)...);
  };

  ASSERT_ASYNC_COMPLETION(resolve_async_exceptional(supply));
}

/// Resolves the given promise asynchonously through an exception
template <typename S>
cti::continuable<> resolve_coro_exceptional(S&& supplier) {
  // Pseudo wait
  co_await supplier();

  throw await_exception{};

  co_return;
}

TYPED_TEST(single_dimension_tests, are_awaitable_with_exceptions_from_coro) {
  auto const& supply = [&](auto&&... args) {
    // Supplies the current tested continuable
    return this->supply(std::forward<decltype(args)>(args)...);
  };

  ASSERT_ASYNC_EXCEPTION_RESULT(resolve_coro_exceptional(supply),
                                await_exception{})
}

#endif // CONTINUABLE_WITH_NO_EXCEPTIONS

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
