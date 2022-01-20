
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

#ifndef CONTINUABLE_DETAIL_CONTAINER_CATEGORY_HPP_INCLUDED
#define CONTINUABLE_DETAIL_CONTAINER_CATEGORY_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace traversal {
/// Deduces to a true type if the given parameter T
/// has a begin() and end() method.
// TODO Find out whether we should use std::begin and std::end instead, which
// could cause issues with plain arrays.
template <typename T, typename = void>
struct is_range : std::false_type {};
template <typename T>
struct is_range<T, traits::void_t<decltype(std::declval<T>().begin() ==
                                           std::declval<T>().end())>>
    : std::true_type {};

/// Deduces to a true type if the given parameter T
/// is accessible through std::tuple_size.
template <typename T, typename = void>
struct is_tuple_like : std::false_type {};
template <typename T>
struct is_tuple_like<T, traits::void_t<decltype(std::tuple_size<T>::value)>>
    : std::true_type {};

/// A tag for dispatching based on the tuple like
/// or container properties of a type.
///
/// This type deduces to a true_type if it has any category.
template <bool IsContainer, bool IsTupleLike>
struct container_category_tag
    : std::integral_constant<bool, IsContainer || IsTupleLike> {};

/// Deduces to the container_category_tag of the given type T.
template <typename T>
using container_category_of_t =
    container_category_tag<is_range<T>::value, is_tuple_like<T>::value>;
} // namespace traversal
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_CONTAINER_CATEGORY_HPP_INCLUDED
