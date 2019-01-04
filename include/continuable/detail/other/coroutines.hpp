
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

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

// Exclude this header when coroutines are not available
#ifndef CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED
#define CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED

#include <cassert>
#include <type_traits>
#include <experimental/coroutine>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_HAS_EXCEPTIONS

namespace cti {
namespace detail {
namespace awaiting {
/// We import the coroutine handle in our namespace
using std::experimental::coroutine_handle;

template <typename T>
struct result_from_identity;
template <typename... T>
struct result_from_identity<identity<T...>> {
  using result_t = result<T...>;
};

/// An object which provides the internal buffer and helper methods
/// for waiting on a continuable in a stackless coroutine.
template <typename Continuable>
class awaitable {
  using hint_t = decltype(base::annotation_of(identify<Continuable>{}));
  using result_t = typename result_from_identity<hint_t>::result_t;

  /// The continuable which is invoked upon suspension
  Continuable continuable_;
  /// A cache which is used to pass the result of the continuation
  /// to the coroutine.
  result_t result_;

public:
  explicit constexpr awaitable(Continuable&& continuable)
      : continuable_(std::move(continuable)) {

    // If the continuable is ready resolve the result from the
    // continuable immediatly.
    if (base::attorney::is_ready(continuable_)) {
      traits::unpack(
          [&](auto&&... args) {
            resolve(std::forward<decltype(args)>(args)...);
          },
          base::attorney::query(std::move(continuable_)));
    }
  }

  /// Since continuables are evaluated lazily we are not
  /// capable to say whether the resumption will be instantly.
  bool await_ready() const noexcept {
    return !result_.is_empty();
  }

  /// Suspend the current context
  // TODO Convert this to an r-value function once possible
  void await_suspend(coroutine_handle<> h) {
    assert(result_.is_empty());
    // Forward every result to the current awaitable
    std::move(continuable_)
        .next([h, this](auto&&... args) mutable {
          resolve(std::forward<decltype(args)>(args)...);
          h.resume();
        })
        .done();
  }

  /// Resume the coroutine represented by the handle
  typename result_t::value_t await_resume() noexcept(false) {
    if (result_) {
      // When the result was resolved return it
      return std::move(result_).get_value();
    }

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
    std::rethrow_exception(result_.get_exception());
#else  // CONTINUABLE_HAS_EXCEPTIONS
    // Returning error types in await isn't supported as of now
    util::trap();
#endif // CONTINUABLE_HAS_EXCEPTIONS
  }

private:
  /// Resolve the continuation through the result
  template <typename... Args>
  void resolve(Args&&... args) {
    assert(result_.is_empty());
    result_.set_value(std::forward<Args>(args)...);
  }

  /// Resolve the continuation through an error
  void resolve(exception_arg_t, exception_t exception) {
    assert(result_.is_empty());
    result_.set_exception(std::move(exception));
  }
};

/// Converts a continuable into an awaitable object as described by
/// the C++ coroutine TS.
template <typename T>
constexpr auto create_awaiter(T&& continuable) {
  return awaitable<std::decay_t<T>>(std::forward<T>(continuable));
}

/// This makes it possible to take the coroutine_handle over on suspension
struct handle_takeover {
  coroutine_handle<>& handle_;

  bool await_ready() noexcept {
    return false;
  }

  void await_suspend(coroutine_handle<> handle) noexcept {
    handle_ = handle;
  }

  void await_resume() noexcept {
  }
};

/// The type which is passed to the compiler that describes the properties
/// of a continuable_base used as coroutine promise type.
template <typename Continuable, typename Promise, typename... Args>
struct promise_type;

/// Implements the resolving method return_void and return_value accordingly
template <typename Base>
struct promise_resolver_base;

template <typename Continuable, typename Promise>
struct promise_resolver_base<promise_type<Continuable, Promise>> {
  void return_void() {
    auto me = static_cast<promise_type<Continuable, Promise>*>(this);
    me->promise_.set_value();
  }
};
template <typename Continuable, typename Promise, typename T>
struct promise_resolver_base<promise_type<Continuable, Promise, T>> {
  void return_value(T value) {
    auto me = static_cast<promise_type<Continuable, Promise, T>*>(this);
    me->promise_.set_value(std::move(value));
  }
};
template <typename Continuable, typename Promise, typename... Args>
struct promise_resolver_base<promise_type<Continuable, Promise, Args...>> {
  template <typename T>
  void return_value(T&& tuple_like) {
    auto me = static_cast<promise_type<Continuable, Promise, Args...>*>(this);
    traits::unpack(std::move(me->promise_), std::forward<T>(tuple_like));
  }
};

template <typename Continuable, typename Promise, typename... Args>
struct promise_type
    : promise_resolver_base<promise_type<Continuable, Promise, Args...>> {

  coroutine_handle<> handle_;
  Promise promise_;

  explicit promise_type() : promise_(types::promise_no_init_arg_t{}) {
  }

  Continuable get_return_object() {
    return [this](auto&& promise) {
      promise_ = std::forward<decltype(promise)>(promise);
      handle_.resume();
    };
  }

  handle_takeover initial_suspend() {
    return {handle_};
  }

  std::experimental::suspend_never final_suspend() {
    return {};
  }

  void unhandled_exception() noexcept {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
    promise_.set_exception(std::current_exception());
#else  // CONTINUABLE_HAS_EXCEPTIONS
       // Returning error types from coroutines isn't supported
    cti::detail::util::trap();
#endif // CONTINUABLE_HAS_EXCEPTIONS
  }
};
} // namespace awaiting
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED
