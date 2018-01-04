
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

#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS
#include <exception>
#endif // CONTINUABLE_WITH_NO_EXCEPTIONS

#include <tuple>

#include "test-continuable.hpp"

namespace std {
namespace experimental {
template <class... T>
struct coroutine_traits<void, T...> {
  struct promise_type {
    void get_return_object() {
    }

    void set_exception(exception_ptr const&) noexcept {
    }

    suspend_never initial_suspend() noexcept {
      return {};
    }

    suspend_never final_suspend() noexcept {
      return {};
    }

    void return_void() noexcept {
    }
  };
};
} // namespace experimental
} // namespace std

/// Resolves the given promise asynchonously
template <typename S, typename T>
void resolve_async(S&& supplier, T&& promise) {
  // 0 args
  co_await supplier();

  // 1 args
  int a1 = co_await supplier(1);
  EXPECT_EQ(a1, 1);

  // 2-n args
  std::tuple<int, int> a2 = co_await supplier(1, 2);
  EXPECT_EQ(a2, std::make_tuple(1, 2));

  promise.set_value();
  co_return;
}

TYPED_TEST(single_dimension_tests, are_awaitable) {
  auto const& supply = [&](auto&&... args) {
    // Supplies the current tested continuable
    return this->supply(std::forward<decltype(args)>(args)...);
  };

  EXPECT_ASYNC_RESULT(
      this->supply().then(cti::make_continuable<void>([&](auto&& promise) {
        // Resolve the cotinuable through a coroutine
        resolve_async(supply, std::forward<decltype(promise)>(promise));
      })));
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
template <typename S, typename T>
void resolve_async_exceptional(S&& supplier, T&& promise) {
  // 0 args
  co_await supplier();

  // 1 args
  int a1 = co_await supplier(1);
  EXPECT_EQ(a1, 1);

  // 2-n args
  std::tuple<int, int> a2 = co_await supplier(1, 2);
  EXPECT_EQ(a2, std::make_tuple(1, 2));

  ASSERT_THROW(co_await supplier().then([] { throw await_exception{}; }),
               await_exception);

  promise.set_value();
  co_return;
}

TYPED_TEST(single_dimension_tests, are_awaitable_with_exceptions) {
  auto const& supply = [&](auto&&... args) {
    // Supplies the current tested continuable
    return this->supply(std::forward<decltype(args)>(args)...);
  };

  ASSERT_ASYNC_COMPLETION(
      this->supply().then(cti::make_continuable<void>([&](auto&& promise) {
        // Resolve the cotinuable through a coroutine
        resolve_async_exceptional(supply,
                                  std::forward<decltype(promise)>(promise));
      })));
}

// TODO Implement this later
//
//  static cti::continuable<int> async_await() {
//   co_await cti::make_continuable<void>([](auto&& promise) {
//      ...
//     promise.set_value();
//   });
//
//   co_return 1;
// }

#endif // CONTINUABLE_WITH_NO_EXCEPTIONS

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
