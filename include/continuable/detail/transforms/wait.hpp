
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

#ifndef CONTINUABLE_DETAIL_TRANSFORMS_WAIT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TRANSFORMS_WAIT_HPP_INCLUDED

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/util.hpp>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#  include <exception>
#endif

namespace cti {
namespace detail {
namespace transforms {

/*
template <typename... Args>
struct sync_trait {
  /// The promise type used to create the future
  using promise_t = std::tuple<Args...>;
  /// Boxes the argument pack into a tuple
  static void resolve(promise_t& promise, Args... args) {
    promise.set_value(std::make_tuple(std::move(args)...));
  }
};
template <>
struct sync_trait<> {
  /// The promise type used to create the future
  using promise_t = result<Args...>;
  /// Boxes the argument pack into void
  static void resolve(promise_t& promise) {
    promise.set_value();
  }
};
template <typename First>
struct sync_trait<First> {
  /// The promise type used to create the future
  using promise_t = std::promise<First>;
  /// Boxes the argument pack into nothing
  static void resolve(promise_t& promise, First first) {
    promise.set_value(std::move(first));
  }
};
*/

template <typename Hint>
struct sync_trait;
template <typename... Args>
struct sync_trait<identity<Args...>> {
  using result_t = result<Args...>;
};

/// Transforms the continuation to sync execution
template <typename Data, typename Annotation>
auto wait(continuable_base<Data, Annotation>&& continuable) {
  constexpr auto hint = base::annotation_of(identify<decltype(continuable)>{});
  using result_t = typename sync_trait<std::decay_t<decltype(hint)>>::result_t;
  (void)hint;

  std::recursive_mutex mutex;
  std::condition_variable_any cv;
  std::atomic_bool ready{false};
  result_t sync_result;

  std::move(continuable)
      .next([&](auto&&... args) {
        sync_result = result_t::from(std::forward<decltype(args)>(args)...);

        ready.store(true, std::memory_order_release);
        cv.notify_all();
      })
      .done();

  if (!ready.load(std::memory_order_acquire)) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    cv.wait(lock, [&] {
      return ready.load(std::memory_order_acquire);
    });
  }

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
  if (sync_result.is_value()) {
    return std::move(sync_result).get_value();
  } else {
    assert(sync_result.is_exception());
    std::rethrow_exception(sync_result.get_exception());
  }
#else
  return sync_result;
#endif // CONTINUABLE_HAS_EXCEPTIONS
}
} // namespace transforms
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TRANSFORMS_WAIT_HPP_INCLUDED
