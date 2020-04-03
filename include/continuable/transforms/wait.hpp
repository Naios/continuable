
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

#ifndef CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED
#define CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED

#include <utility>
#include <continuable/detail/transforms/wait.hpp>

namespace cti {
/// \ingroup Transforms
/// \{

namespace transforms {
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
/// \attention If exceptions are used, exceptions that are thrown, are rethrown
///            synchronously.
///
/// \since 4.0.0
inline auto wait() {
  return [](auto&& continuable) {
    return detail::transforms::wait(
        std::forward<decltype(continuable)>(continuable));
  };
}
} // namespace transforms
/// \}
} // namespace cti

#endif // CONTINUABLE_TRANSFORMS_WAIT_HPP_INCLUDED
