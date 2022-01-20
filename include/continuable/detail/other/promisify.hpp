
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

#ifndef CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED
#define CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED

#include <type_traits>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_HAS_EXCEPTIONS

namespace cti {
namespace detail {
namespace convert {
/// A resolver for promisifying asio and js style callbacks.
inline auto default_resolver() {
  return [](auto&& promise, auto&& e, auto&&... args) {
    static_assert(
        std::is_convertible<std::decay_t<decltype(e)>, exception_t>::value,
        "The given error type must be convertible to the error type used! "
        "Specify a custom resolver in order to apply a conversion to the "
        "used error type.");

    if (e) {
      promise.set_exception(std::forward<decltype(e)>(e));
    } else {
      promise.set_value(std::forward<decltype(args)>(args)...);
    }
  };
}

template <typename... Result>
struct promisify_helper {
  template <typename Resolver, typename Callable, typename... Args>
  static auto from(Resolver&& resolver, Callable&& callable, Args&&... args) {
    return make_continuable<Result...>(
        [resolver = std::forward<Resolver>(resolver),
         args = traits::make_flat_tuple(std::forward<Callable>(callable),
                                        std::forward<Args>(args)...)](
            auto&& promise) mutable {
          traits::unpack(
              [promise = std::forward<decltype(promise)>(promise),
               &resolver](auto&&... args) mutable {
                // Call the resolver from with the promise and result
                auto callback =
                    [resolver = std::move(resolver),
                     promise = std::move(promise)](auto&&... args) mutable {
                      resolver(std::move(promise),
                               std::forward<decltype(args)>(args)...);
                    };

                // Invoke the callback taking function
                util::invoke(std::forward<decltype(args)>(args)...,
                             std::move(callback));
              },
              std::move(args));
        });
  }
};
} // namespace convert
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_PROMISIFY_HPP_INCLUDED
