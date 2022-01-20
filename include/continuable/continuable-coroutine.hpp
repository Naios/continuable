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

#ifndef CONTINUABLE_COROUTINE_HPP_INCLUDED
#define CONTINUABLE_COROUTINE_HPP_INCLUDED

#include <continuable/continuable-base.hpp>
#include <continuable/continuable-types.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#  include <exception>
#endif // CONTINUABLE_HAS_EXCEPTIONS

#if defined(CONTINUABLE_HAS_COROUTINE)
#  include <continuable/detail/other/coroutines.hpp>

namespace cti {
#  if defined(CONTINUABLE_HAS_EXCEPTIONS)
/// Is thrown from co_await expressions if the awaited continuable is canceled
///
/// Default constructed exception types that are returned by a cancelled
/// continuable are converted automatically to await_canceled_exception when
/// being returned by a co_await expression.
///
/// The await_canceled_exception gets converted again to a default constructed
/// exception type if it becomes unhandled inside a coroutine which
/// returns a continuable_base.
/// ```cpp
/// continuable<> cancelled_coroutine() {
///   co_await make_cancelling_continuable<void>();
///
///   co_return;
/// }
///
/// // ...
///
/// cancelled_coroutine().fail([](exception_t e) {
///   assert(bool(e) == false);
/// });
/// ```
///
/// \since 4.1.0
using await_canceled_exception = detail::awaiting::await_canceled_exception;
#  endif // CONTINUABLE_HAS_EXCEPTIONS
} // namespace cti

/// \cond false
// As far as I know there is no other way to implement this specialization...
// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {
#  if defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)
namespace experimental {
#  endif // defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)
template <typename Data, typename... Args, typename... FunctionArgs>
struct coroutine_traits<
    cti::continuable_base<Data, cti::detail::identity<Args...>>,
    FunctionArgs...> {

  using promise_type = cti::detail::awaiting::promise_type<
      cti::continuable<Args...>, cti::promise<Args...>, Args...>;
};
#  if defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)
} // namespace experimental
#  endif // defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)
} // namespace std
  /// \endcond
#endif // defined(CONTINUABLE_HAS_COROUTINE)

#endif // CONTINUABLE_COROUTINE_HPP_INCLUDED
