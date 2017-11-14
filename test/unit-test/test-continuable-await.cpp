
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

#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

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

TYPED_TEST(single_dimension_tests, is_awaitable) {

  {}
}

auto mk() {
  return cti::make_continuable<void>([](auto&& promise) {
    // ...
    promise.set_value();
  });
}

void teststhh() {
  auto c = mk();

  co_await std::move(c);
}

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
