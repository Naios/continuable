
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

#ifndef CONTINUABLE_TRANSFORMS_FUTURE_HPP_INCLUDED
#define CONTINUABLE_TRANSFORMS_FUTURE_HPP_INCLUDED

#include <utility>
#include <continuable/detail/transforms/future.hpp>

namespace cti {
/// \ingroup Transforms
/// \{

namespace transforms {
/// Returns a transform that if applied to a continuable,
/// it will start the continuation chain and returns the asynchronous
/// result as `std::future<...>`.
///
/// \returns Returns a `std::future<...>` which becomes ready as soon
///          as the the continuation chain has finished.
///          The signature of the future depends on the result type:
/// |          Continuation type        |             Return type            |
/// | : ------------------------------- | : -------------------------------- |
/// | `continuable_base with <>`        | `std::future<void>`                |
/// | `continuable_base with <Arg>`     | `std::future<Arg>`                 |
/// | `continuable_base with <Args...>` | `std::future<std::tuple<Args...>>` |
///
/// \attention If exceptions are used, exceptions that are thrown, are forwarded
///            to the returned future. If there are no exceptions supported,
///            you shall not pass any errors to the end of the asynchronous
///            call chain!
///            Otherwise this will yield a trap that causes application exit.
///
/// \since 2.0.0
inline auto to_future() {
  return [](auto&& continuable) {
    return detail::transforms::to_future(
        std::forward<decltype(continuable)>(continuable));
  };
}
} // namespace transforms
/// \}
} // namespace cti

#endif // CONTINUABLE_OPERATIONS_SPLIT_HPP_INCLUDED
