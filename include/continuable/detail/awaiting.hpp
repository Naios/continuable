
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

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

#ifndef CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED__

// Exlude this header when coroutines are not available
#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

#include <cassert>
#include <experimental/coroutine>

#include <continuable/continuable-api.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/expected.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

#if defined(CONTINUABLE_WITH_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_WITH_EXCEPTIONS

namespace cti {
namespace detail {
namespace awaiting {
namespace detail {
struct void_guard_tag {};

template <typename T>
struct result_trait;
template <>
struct result_trait<traits::identity<>> {
  using expected = util::expected<void_guard_tag>;

  static constexpr void_guard_tag wrap() noexcept {
    return {};
  }
  static void unwrap(expected&& e) {
    assert(e.is_value());
    (void)e;
  }
};
template <typename T>
struct result_trait<traits::identity<T>> {
  using expected = util::expected<T>;

  static auto wrap(T arg) {
    return std::move(arg);
  }
  static auto unwrap(expected&& e) {
    assert(e.is_value());
    return std::move(e.get_value());
  }
};
template <typename First, typename Second, typename... Rest>
struct result_trait<traits::identity<First, Second, Rest...>> {
  using expected = util::expected<std::tuple<First, Second, Rest...>>;

  static auto wrap(First first, Second second, Rest... rest) {
    return std::make_tuple(std::move(first), std::move(second),
                           std::move(rest)...);
  }
  static auto unwrap(expected&& e) {
    assert(e.is_value());
    return std::move(e.get_value());
  }
};

template <typename Continuable>
using result_trait_t =
    result_trait<decltype(base::hint_of(traits::identity_of<Continuable>()))>;
} // namespace detail

/// We import the coroutine handle in our namespace
using std::experimental::coroutine_handle;

/// An object which provides the internal buffer and helper methods
/// for waiting on a continuable in a stackless coroutine.
template <typename Continuable>
class awaitable {
  using trait_t = detail::result_trait_t<Continuable>;

  /// The continuable which is invoked upon suspension
  Continuable continuable_;
  /// A cache which is used to pass the result of the continuation
  /// to the coroutine.
  typename trait_t::expected result_;

public:
  explicit awaitable(Continuable&& continuable)
      : continuable_(std::move(continuable)) {
  }

  /// Since continuables are evaluated lazily we are not
  /// capable to say whether the resumption will be instantly.
  bool await_ready() const noexcept {
    return false;
  }

  /// Suspend the current context
  // TODO Convert this to an r-value function once possible
  void await_suspend(coroutine_handle<> h) {
    // Forward every result to the current awaitable
    std::move(continuable_)
        .next([h, this](auto&&... args) {
          resolve(std::forward<decltype(args)>(args)...);
          h.resume();
        })
        .done();
  }

  /// Resume the coroutine represented by the handle
  auto await_resume() noexcept(false) {
    if (result_) {
      // When the result was resolved return it
      return trait_t::unwrap(std::move(result_));
    }

#if defined(CONTINUABLE_WITH_EXCEPTIONS)
    std::rethrow_exception(result_.get_exception());
#else  // CONTINUABLE_WITH_EXCEPTIONS
    // Returning error types in await isn't supported as of now
    util::trap();
#endif // CONTINUABLE_WITH_EXCEPTIONS
  }

private:
  /// Resolve the continuation through the result
  template <typename... Args>
  void resolve(Args&&... args) {
    result_.set_value(trait_t::wrap(std::forward<Args>(args)...));
  }

  /// Resolve the continuation through an error
  void resolve(types::dispatch_error_tag, types::error_type error) {
    result_.set_exception(std::move(error));
  }
};

/// Converts a continuable into an awaitable object as described by
/// the C++ coroutine TS.
template <typename T>
auto create_awaiter(T&& continuable) {
  return awaitable<std::decay_t<T>>(std::forward<T>(continuable));
}
} // namespace awaiting
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
#endif // CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED__
