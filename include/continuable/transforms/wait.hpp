
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

#ifndef CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED
#define CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED

#include <chrono>
#include <condition_variable>
#include <utility>
#include <continuable/detail/features.hpp>
#include <continuable/detail/transforms/wait.hpp>

namespace cti {
/// \ingroup Transforms
/// \{

namespace transforms {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
/// Is thrown from wait if the awaited continuable_base was cancelled,
/// which was signaled through resolving with a default
/// constructed exception type.
///
/// \since 4.1.0
using wait_transform_canceled_exception = detail::transforms::wait_transform_canceled_exception;
#endif // CONTINUABLE_HAS_EXCEPTIONS

/// Returns a transform that if applied to a continuable,
/// it will start the continuation chain and returns the result synchronously.
/// The current thread is blocked until the continuation chain is finished.
///
/// \returns Returns a value that is available immediately.
///          The signature of the future depends on the result type:
/// |          Continuation type        |             Return type            |
/// | : ------------------------------- | : -------------------------------- |
/// | `continuable_base with <>`        | `void`                             |
/// | `continuable_base with <Arg>`     | `Arg`                              |
/// | `continuable_base with <Args...>` | `std::tuple<Args...>`              |
///
/// \throws wait_transform_canceled_exception if the awaited continuable_base
///         is cancelled, and thus was resolved with a default
///         constructed exception type.
///
/// \attention If exceptions are used, exceptions that are thrown, are rethrown
///            synchronously.
///
/// \since 4.0.0
inline auto wait() {
  return [](auto&& continuable) {
    return detail::transforms::wait_and_unpack(
        std::forward<decltype(continuable)>(continuable));
  };
}

/// \copybrief wait
///
/// \returns Returns a result that is available immediately.
///          The signature of the future depends on the result type:
/// |          Continuation type        |             Return type            |
/// | : ------------------------------- | : -------------------------------- |
/// | `continuable_base with <>`        | `result<>`                         |
/// | `continuable_base with <Arg>`     | `result<Arg>`                      |
/// | `continuable_base with <Args...>` | `result<Args...>`                  |
///
/// \attention Thrown exceptions returned through the result, also
///            make sure to check for a valid result value in case the
///            underlying time constraint timed out.
///
/// \since 4.0.0
template <typename Rep, typename Period>
auto wait_for(std::chrono::duration<Rep, Period> duration) {
  return [duration](auto&& continuable) {
    return detail::transforms::wait_unsafe(
        std::forward<decltype(continuable)>(continuable),
        [duration](detail::transforms::condition_variable_t& cv,
                   detail::transforms::lock_t& lock, auto&& predicate) {
          cv.wait_for(lock, duration,
                      std::forward<decltype(predicate)>(predicate));
        });
  };
}

/// \copydoc wait_for
template <typename Clock, typename Duration>
auto wait_until(std::chrono::time_point<Clock, Duration> time_point) {
  return [time_point](auto&& continuable) {
    return detail::transforms::wait_unsafe(
        std::forward<decltype(continuable)>(continuable),
        [time_point](detail::transforms::condition_variable_t& cv,
                     detail::transforms::lock_t& lock, auto&& predicate) {
          cv.wait_until(lock, time_point,
                        std::forward<decltype(predicate)>(predicate));
        });
  };
}
} // namespace transforms
/// \}
} // namespace cti

#endif // CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED
