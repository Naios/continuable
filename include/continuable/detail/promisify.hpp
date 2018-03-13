
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

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

#ifndef CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED
#define CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED

#include <type_traits>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_HAS_EXCEPTIONS

#include <continuable/continuable-base.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
namespace detail {
namespace convert {
/// A helper class for promisifying asio and js style callback
/// taking functions into a continuable.
template <typename P>
struct promisify_default {
  P promise;

  template <typename E, typename... T>
  void operator()(E&& error, T&&... result) {
    if (error) {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
      promise.set_exception(std::make_exception_ptr(std::forward<E>(error)));
#else
      promise.set_exception(
          std::error_condition(error.value(), error.category()));
#endif // CONTINUABLE_HAS_EXCEPTIONS

    } else {
      promise.set_value(std::forward<T>(result)...);
    }
  }
};

template <typename... Result>
struct promisify_helper {
  template <template <class T> class Evaluator, typename Callable,
            typename... Args>
  static auto from(Callable&& callable, Args&&... args) {
    return make_continuable<Result...>([args = std::make_tuple(
                                            std::forward<Callable>(callable),
                                            std::forward<Args>(args)...)](
        auto&& promise) mutable {

      traits::unpack(
          [promise = std::forward<decltype(promise)>(promise)](
              auto&&... args) mutable {
            Evaluator<std::decay_t<decltype(promise)>> evaluator{
                std::move(promise)};

            util::invoke(std::forward<decltype(args)>(args)...,
                         std::move(evaluator));
          },
          std::move(args));
    });
  }
};
} // namespace convert
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED
