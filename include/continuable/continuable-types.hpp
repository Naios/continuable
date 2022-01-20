
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

#ifndef CONTINUABLE_TYPES_HPP_INCLUDED
#define CONTINUABLE_TYPES_HPP_INCLUDED

#include <function2/function2.hpp>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-promise-base.hpp>
#include <continuable/detail/other/erasure.hpp>

namespace cti {
/// \defgroup Types Types
/// provides the \link cti::continuable continuable\endlink and \link
/// cti::promise promise\endlink facility for type erasure.
/// \{

/// Deduces to the preferred continuation capacity for a possible
/// small functor optimization. The given capacity size is always enough to
/// to avoid any allocation when storing a ready continuable_base.
///
/// \since 4.0.0
template <typename... Args>
using continuation_capacity = detail::erasure::continuation_capacity<Args...>;

/// Defines a non-copyable continuation type which uses the
/// function2 backend for type erasure.
///
/// Usable like: `continuable<int, float>`
///
/// \note You can always define your own continuable with a type erasure of
///       choice, the type erasure wrapper just needs to accept a
///       callable object with a continuation signature as specified
///       in the Primitives section.
///
/// \since 1.0.0
template <typename... Args>
using continuable = continuable_base<detail::erasure::continuation<Args...>, //
                                     signature_arg_t<Args...>>;

/// Defines a non-copyable promise type which is using the
/// function2 backend for type erasure.
///
/// Usable like: `promise<int, float>`
///
/// \note You can always define your own promise with a type erasure of
///       choice, the type erasure wrapper just needs to accept a
///       callable object with a callback signature as specified
///       in the Primitives section.
///
/// \since 1.0.0
template <typename... Args>
using promise = promise_base<detail::erasure::callback<Args...>, //
                             signature_arg_t<Args...>>;

/// Defines a non-copyable type erasure which is capable of carrying
/// callable objects passed to executors.
///
/// The work behaves like a `promise<>` but the work type erasure uses extra
/// stack space for small object optimization.
/// Additionally the outstanding work can be resolved through an exception.
///
/// \note You can always define your own cancelable_work with a type erasure of
///       choice, the type erasure wrapper just needs to accept a
///       callable object which is callable with a `void()` and
///       `void(exception_arg_t, exception_t)` signature.
///
/// \since 4.0.0
using work = promise_base<detail::erasure::work, //
                          signature_arg_t<>>;
/// \}
} // namespace cti

#endif // CONTINUABLE_TYPES_HPP_INCLUDED
