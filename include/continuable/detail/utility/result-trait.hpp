
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

#ifndef CONTINUABLE_DETAIL_RESULT_TRAIT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_RESULT_TRAIT_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
struct void_arg_t { };

template <typename... T>
struct result_trait;
template <>
struct result_trait<> {
  using value_t = void;
  using surrogate_t = void_arg_t;

  static constexpr surrogate_t wrap() noexcept {
    return {};
  }

  static constexpr void unwrap(surrogate_t) {
  }
};
template <typename T>
struct result_trait<T> {
  using value_t = T;
  using surrogate_t = value_t;

  static surrogate_t wrap(T arg) {
    return std::move(arg);
  }

  template <typename R>
  static decltype(auto) unwrap(R&& unwrap) {
    return std::forward<R>(unwrap);
  }

  template <std::size_t I, typename Result>
  static decltype(auto) get(Result&& result) {
    return std::forward<Result>(result).get_value();
  }
};
template <typename First, typename Second, typename... Rest>
struct result_trait<First, Second, Rest...> {
  using value_t = std::tuple<First, Second, Rest...>;
  using surrogate_t = value_t;

  static surrogate_t wrap(First first, Second second, Rest... rest) {
    return std::make_tuple(std::move(first), std::move(second),
                           std::move(rest)...);
  }

  template <typename R>
  static decltype(auto) unwrap(R&& unwrap) {
    return std::forward<R>(unwrap);
  }

  template <std::size_t I, typename Result>
  static decltype(auto) get(Result&& result) {
    return std::get<I>(std::forward<Result>(result).get_value());
  }
};
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_RESULT_TRAIT_HPP_INCLUDED
