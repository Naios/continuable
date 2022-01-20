
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

#ifndef CONTINUABLE_OPERATIONS_LOOP_HPP_INCLUDED
#define CONTINUABLE_OPERATIONS_LOOP_HPP_INCLUDED

#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/detail/operations/loop.hpp>

namespace cti {
/// \ingroup Operations
/// \{

/// Can be used to create an asynchronous loop.
///
/// The callable will be called repeatedly until it returns a
/// cti::continuable_base which then resolves to a present cti::result.
///
/// For better readability cti::loop_result, cti::loop_break and
/// cti::loop_continue are provided which can be used as following:
/// ```cpp
/// auto while_answer_not_yes() {
///   return loop([] {
///     return ask_something().then([](std::string answer) -> loop_result<> {
///       if (answer == "yes") {
///         return loop_break();
///       } else {
///         return loop_continue();
///       }
///     });
///   });
/// }
/// ```
///
/// \param callable The callable type which must return a cti::continuable_base
///        which then resolves to a cti::result of arbitrary values.
///
/// \param args The arguments that are passed to the callable upon
///             each invocation.
///
/// \since 4.0.0
///
template <typename Callable, typename... Args>
auto loop(Callable&& callable, Args&&... args) {
  return detail::operations::loop(std::forward<Callable>(callable),
                                  std::forward<Args>(args)...);
}

/// Can be used to indicate a specific result inside an asynchronous loop.
///
/// See cti::loop for details.
///
/// \since 4.0.0
template <typename... T>
using loop_result = plain_t<result<T...>>;

/// Can be used to create a loop_result which causes the loop to be
/// cancelled and resolved with the given arguments.
///
/// See cti::loop for details.
///
/// \since 4.0.0
template <typename... T>
auto loop_break(T&&... args) {
  return make_plain(make_result(std::forward<T>(args)...));
}

/// Can be used to create a loop_result which causes the loop to be repeated.
///
/// See cti::loop for details.
///
/// \since 4.0.0
inline auto loop_continue() noexcept {
  return empty_result{};
}

/// Can be used to create an asynchronous loop over a specific range.
///
/// The callable will be called repeatedly with each with begin increased
/// until end is reached.
///
/// ```cpp
/// auto iterate_some() {
///   // Iterate step from 0 to 9
///   return range_loop([] (int step) {
///     return do_something(i).then([] {
///       // You don't have to return a result here
///     });
///   }, 0, 10);
/// }
/// ```
///
/// \param callable The callable type which must return a cti::continuable_base
///        which then resolves to a cti::result of arbitrary values.
///
/// \param begin The iterator to iterate over
///
/// \param end The iterator to iterate until
///
/// \since 4.0.0
///
template <typename Callable, typename Iterator>
auto range_loop(Callable&& callable, Iterator begin, Iterator end) {
  return detail::operations::loop( //
      detail::operations::make_range_looper(std::forward<Callable>(callable),
                                            begin, end));
}

/// \}
} // namespace cti

#endif // CONTINUABLE_OPERATIONS_LOOP_HPP_INCLUDED
