
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

#ifndef CONTINUABLE_OPERATIONS_SPLIT_HPP_INCLUDED
#define CONTINUABLE_OPERATIONS_SPLIT_HPP_INCLUDED

#include <utility>
#include <continuable/detail/operations/split.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
/// \ingroup Operations
/// \{

/// Splits the asynchronous control flow and merges multiple promises/callbacks
/// together, which take the same types of arguments, into one.
///
/// The invocation order of all promises is undefined.
///
/// The split function is the opposite of the connection functions
/// like `when_all` because is can merge multiple waiters together rather than
/// joining those.
///
/// The split function can be used to resolve multiple waiters when resolving
/// a single promise.
/// ```cpp
/// class my_class {
///   cti::promise<> promise_;
///
/// public:
///   cti::continuable<> wait_for_sth() {
///     return [this](auto&& promise) mutable {
///       // Make sure accessing promise_ is done in a thread safe way!
///       promise_ = cti::split(std::move(promise_),
///                             std::forward<decltype(promise)>(promise));
///     };
///   }
///
///   void resolve_all() {
///     // Resolves all waiting promises
///     promise_.set_value();
///   }
/// };
/// ```
///
/// \note The split function only works if all asynchronous arguments are
///       copyable. All asynchronous arguments and exceptions will be passed
///       to all split promises.
///
/// \param promises The promises to split the control flow into,
///                 can be single promises or heterogeneous or homogeneous
///                 containers of promises (see traverse_pack for a description
///                 of supported nested arguments).
///
/// \returns A new promise with the same asynchronous result types as
///          the given promises.
///
/// \since 4.0.0
///
template <typename... Promises>
auto split(Promises&&... promises) {
  return detail::operations::split_promise<
      detail::traits::unrefcv_t<Promises>...>(
      std::forward<Promises>(promises)...);
}
/// \}
} // namespace cti

#endif // CONTINUABLE_OPERATIONS_SPLIT_HPP_INCLUDED
