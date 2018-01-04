
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_TRANSFORMS_HPP_INCLUDED__
#define CONTINUABLE_TRANSFORMS_HPP_INCLUDED__

#include <continuable/continuable-api.hpp>
#include <continuable/detail/transforms.hpp>

namespace cti {
/// The namespace transforms declares callable objects that transform
/// any continuable_base to an object or to a continuable_base itself.
///
/// Transforms can be applied to continuables through using
/// the continuable-base::apply method accordingly.
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
/// \since version 2.0.0
inline auto futurize() {
  return [](auto&& continuable) {
    using detail::transforms::as_future;
    return as_future(std::forward<decltype(continuable)>(continuable));
  };
}

/// Returns a transform that if applied to a continuable, it will ignores all
/// error which ocured until the point the transform was applied.
///
/// \returns Returns a continuable with the same signature as applied to.
///
/// \attention This can be used to create a continuable which doesn't resolve
///            the continuation on errors.
///
/// \since version 2.0.0
inline auto flatten() {
  return [](auto&& continuable) {
    return std::forward<decltype(continuable)>(continuable).fail([](auto&&) {});
  };
}
} // namespace transforms
} // namespace cti

#endif // CONTINUABLE_TRANSFORMS_HPP_INCLUDED__
