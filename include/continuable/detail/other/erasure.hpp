
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

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

#ifndef CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED

#include <type_traits>
#include <function2/function2.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
namespace erasure {
/*template <typename... Args>
using continuation_capacity = std::integral_constant<
    std::size_t, (sizeof(base::ready_continuation<Args...>) < sizeof(void*)
                      ? sizeof(void*)
                      : sizeof(base::ready_continuation<Args...>))>;*/

template <std::size_t Size>
struct sized {
  template <typename... Args>
  using erasure_base_t =                    //
      fu2::function_base<true, false, Size, //
                         true, false, Args...>;
};

template <template <typename...> class Wrapper, typename... Args>
using callback_of_t = Wrapper<void(Args...)&&, //
                              void(exception_arg_t, exception_t) &&>;

template <typename... Args>
using callback_base_t = callback_of_t<sized<0>::erasure_base_t, Args...>;

template <typename... Args>
class callback : public callback_base_t<Args...> {
public:
  using callback_base_t<Args...>::callback_base_t;
  using callback_base_t<Args...>::operator=;
  using callback_base_t<Args...>::operator();
};

template <template <typename...> class Wrapper, typename Promise,
          typename... Args>
using continuation_of_t = Wrapper<void(Promise),              //
                                  bool(is_ready_arg_t) const, //
                                  std::tuple<Args...>(query_arg_t)>;

template <typename... Args>
using continuation_base_t =
    continuation_of_t<sized<0 /*
          continuation_capacity<Args...>::value*/>::erasure_base_t,
                      promise_base<callback<Args...>, signature_arg_t<Args>...>,
                      Args...>;

template <typename... Args>
class continuation : public continuation_base_t<Args...> {
public:
  using continuation_base_t<Args...>::callback_base_t;
  using continuation_base_t<Args...>::operator=;
  using continuation_base_t<Args...>::operator();
};
} // namespace erasure
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED
