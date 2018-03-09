
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

#ifndef CONTINUABLE_TYPES_HPP_INCLUDED
#define CONTINUABLE_TYPES_HPP_INCLUDED

#include <cstdint>

#include <function2/function2.hpp>

#include <continuable/continuable-trait.hpp>

namespace cti {
// clang-format off
namespace detail {
/// A function which isn't size adjusted and move only
template<std::size_t, typename... Args>
using unique_function_adapter = fu2::unique_function<Args...>;
/// A function which is size adjusted and move only
template<std::size_t Size, typename... Args>
using unique_function_adjustable = fu2::function_base<true, false, Size,
                                                      true, false, Args...>;

/// We adjust the internal capacity of the outer function wrapper so
/// we don't have to allocate twice when using `continuable<...>`.
template<typename... Args>
using unique_trait_of = continuable_trait<
  unique_function_adapter,
  unique_function_adjustable,
  Args...
>;
} // namespace detail

/// Defines a non-copyable continuation type which uses the
/// function2 backend for type erasure.
///
/// Usable like: `continuable<int, float>`
template <typename... Args>
using continuable = typename detail::unique_trait_of<
  Args...
>::continuable;

/// Defines a non-copyable promise type which is using the
/// function2 backend for type erasure.
///
/// Usable like: `promise<int, float>`
template <typename... Args>
using promise = typename detail::unique_trait_of<
  Args...
>::promise;

// TODO channel
// TODO sink

// clang-format on
} // namespace cti

#endif // CONTINUABLE_TYPES_HPP_INCLUDED
