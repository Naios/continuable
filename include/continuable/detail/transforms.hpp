
/**

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

#ifndef CONTINUABLE_DETAIL_TRANSFORMS_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_TRANSFORMS_HPP_INCLUDED__

#include <future>

#include <continuable/detail/api.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/types.hpp>

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
class promise_callback<hints::signature_hint_tag<Args...>>
    : public future_trait<Args...> {

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
#if !defined(CONTINUABLE_WITH_CUSTOM_ERROR_TYPE) &&                            \
    !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
  /// Resolves the promise through the exception
  void operator()(types::dispatch_error_tag, types::error_type error) {
    promise_.set_exception(error);
  }
#endif

  /// Returns the future from the promise
  auto get_future() {
    return promise_.get_future();
  }
};

/// Transforms the continuation to a future
template <typename Data, typename Annotation>
auto as_future(continuable_base<Data, Annotation>&& continuable) {
  // Create the promise which is able to supply the current arguments
  auto hint = base::hint_of(traits::identity_of(continuable));

  promise_callback<std::decay_t<decltype(hint)>> callback;
  (void)hint;

  // Receive the future
  auto future = callback.get_future();

  // Dispatch the continuation with the promise resolving callback
  std::move(continuable).then(std::move(callback)).done();

  return future;
}
} // namespace transforms
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TRANSFORMS_HPP_INCLUDED__
