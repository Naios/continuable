
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

#ifndef CONTINUABLE_OPERATIONS_ASYNC_HPP_INCLUDED
#define CONTINUABLE_OPERATIONS_ASYNC_HPP_INCLUDED

#include <utility>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/operations/async.hpp>

namespace cti {
/// \ingroup Operations
/// \{

/// Wraps the given callable inside a continuable_base such that it is
/// invoked when the asynchronous result is requested to return the result.
///
/// The async function shall be seen as an equivalent to std::async.
///
/// The behaviour will be equal as when using make_ready_continuable together
/// with continuable_base::then, but async is implemented in
/// a more efficient way:
/// ```cpp
/// auto do_sth() {
///   return async([] {
///     do_sth_more();
///     return 0;
///   });
/// }
/// ```
///
/// \param callable The callable type which is invoked on request.
///
/// \param args The arguments which are passed to the callable upon invocation.
///
/// \returns A continuable_base which asynchronous result type will
///          be computed with the same rules as continuable_base::then .
///
/// \since 4.0.0
///
template <typename Callable, typename... Args>
auto async(Callable&& callable, Args&&... args) {
  return detail::operations::async(std::forward<Callable>(callable),
                                   detail::types::this_thread_executor_tag{},
                                   std::forward<Args>(args)...);
}

/// Wraps the given callable inside a continuable_base such that it is
/// invoked through the given executor when the asynchronous result
/// is requested to return the result.
///
/// The behaviour will be equal as when using make_ready_continuable together
/// with continuable_base::then and the given executor but async_on
/// is implemented in a more efficient way:
/// ```cpp
/// auto do_sth() {
///   auto executor = [](auto&& work) {
///     // Do something with the work here
///     std::forward<decltype(work)>(work);
///   };
///
///   return async_on([] {
///     do_sth_more();
///     return 0;
///   }, my_executor);
/// }
/// ```
///
/// \param callable The callable type which is invoked on request.
///
/// \param executor The executor that is used to dispatch the given callable.
///
/// \param args The arguments which are passed to the callable upon invocation.
///
/// \returns A continuable_base which asynchronous result type will
///          be computed with the same rules as continuable_base::then .
///
/// \since 4.0.0
///
template <typename Callable, typename Executor, typename... Args>
auto async_on(Callable&& callable, Executor&& executor, Args&&... args) {
  return detail::operations::async(std::forward<Callable>(callable),
                                   std::forward<Executor>(executor),
                                   std::forward<Args>(args)...);
}
/// \}
} // namespace cti

#endif // CONTINUABLE_OPERATIONS_ASYNC_HPP_INCLUDED
