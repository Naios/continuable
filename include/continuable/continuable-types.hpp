
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

#ifndef CONTINUABLE_TYPES_HPP_INCLUDED
#define CONTINUABLE_TYPES_HPP_INCLUDED

#include <cstddef>
#include <function2/function2.hpp>
#include <continuable/continuable-trait.hpp>

namespace cti {
/// \defgroup Types Types
/// provides the \link cti::continuable continuable\endlink and \link
/// cti::promise promise\endlink facility for type erasure.
/// \{

namespace detail {
/// A type erasure which isn't size adjusted and move only
template <std::size_t, typename... Args>
class type_erasure : public fu2::unique_function<Args...> {
public:
  using fu2::unique_function<Args...>::unique_function;
  using fu2::unique_function<Args...>::operator=;
  using fu2::unique_function<Args...>::operator();
};

/// A function which is size adjusted and move only
template <std::size_t Size, typename... Args>
class sized_type_erasure
    : public fu2::function_base<true, false, Size, true, false, Args...> {

public:
  using fu2::function_base<true, false, Size, //
                           true, false, Args...>::function_base;
  using fu2::function_base<true, false, Size, //
                           true, false, Args...>::operator=;
  using fu2::function_base<true, false, Size, //
                           true, false, Args...>::operator();
};

/// We adjust the internal capacity of the outer function wrapper so
/// we don't have to allocate twice when using `continuable<...>`.
template <typename... Args>
using unique_trait_of = continuable_trait< //
    type_erasure, sized_type_erasure,
    Args... //
    >;

/// A type erasure for work objects
class work_type_erasure : public fu2::unique_function<void()> {
public:
  using fu2::unique_function<void()>::unique_function;
  using fu2::unique_function<void()>::operator=;
  using fu2::unique_function<void()>::operator();
};
} // namespace detail

/// Defines a non-copyable continuation type which uses the
/// function2 backend for type erasure.
///
/// Usable like: `continuable<int, float>`
template <typename... Args>
using continuable = typename detail::unique_trait_of<Args...>::continuable;

/// Defines a non-copyable promise type which is using the
/// function2 backend for type erasure.
///
/// Usable like: `promise<int, float>`
template <typename... Args>
using promise = typename detail::unique_trait_of<Args...>::promise;

/// Defines a non-copyable type erasure which is capable of carrying
/// callable objects passed to executors.
///
/// \since 4.0.0
using work = detail::work_type_erasure;

// TODO channel
// TODO sink
/// \}
} // namespace cti

#endif // CONTINUABLE_TYPES_HPP_INCLUDED
