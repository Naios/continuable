
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.2.0

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

#ifndef CONTINUABLE_DETAIL_TRANSFORMS_FUTURE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TRANSFORMS_FUTURE_HPP_INCLUDED

#include <future>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
/// Provides helper functions to transform continuations to other types
namespace transforms {
/// Provides helper functions and typedefs for converting callback arguments
/// to their types a promise can accept.
template <typename... Args>
struct future_trait {
  /// The promise type used to create the future
  using promise_t = std::promise<std::tuple<Args...>>;
  /// Boxes the argument pack into a tuple
  static void resolve(promise_t& promise, Args... args) {
    promise.set_value(std::make_tuple(std::move(args)...));
  }
};
template <>
struct future_trait<> {
  /// The promise type used to create the future
  using promise_t = std::promise<void>;
  /// Boxes the argument pack into void
  static void resolve(promise_t& promise) {
    promise.set_value();
  }
};
template <typename First>
struct future_trait<First> {
  /// The promise type used to create the future
  using promise_t = std::promise<First>;
  /// Boxes the argument pack into nothing
  static void resolve(promise_t& promise, First first) {
    promise.set_value(std::move(first));
  }
};

template <typename Hint>
class promise_callback;

template <typename... Args>
class promise_callback<identity<Args...>> : public future_trait<Args...> {

  typename future_trait<Args...>::promise_t promise_;

public:
  constexpr promise_callback() = default;
  promise_callback(promise_callback const&) = delete;
  constexpr promise_callback(promise_callback&&) = default;
  promise_callback& operator=(promise_callback const&) = delete;
  promise_callback& operator=(promise_callback&&) = delete;

  /// Resolves the promise
  void operator()(Args... args) {
    this->resolve(promise_, std::move(args)...);
  }

  /// Resolves the promise through the exception
  void operator()(exception_arg_t, exception_t error) {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
    promise_.set_exception(error);
#else
    (void)error;

    // Can't forward a std::error_condition or custom error type
    // to a std::promise. Handle the error first in order
    // to prevent this trap!
    CTI_DETAIL_TRAP();
#endif // CONTINUABLE_HAS_EXCEPTIONS
  }

  /// Returns the future from the promise
  auto get_future() {
    return promise_.get_future();
  }
};

/// Transforms the continuation to a future
template <typename Data, typename Annotation>
auto to_future(continuable_base<Data, Annotation>&& continuable) {
  // Create the promise which is able to supply the current arguments
  constexpr auto const hint =
      base::annotation_of(identify<decltype(continuable)>{});

  promise_callback<std::decay_t<decltype(hint)>> callback;
  (void)hint;

  // Receive the future
  auto future = callback.get_future();

  // Dispatch the continuation with the promise resolving callback
  std::move(continuable).next(std::move(callback)).done();

  return future;
}
} // namespace transforms
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TRANSFORMS_FUTURE_HPP_INCLUDED
