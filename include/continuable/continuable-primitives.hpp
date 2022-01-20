
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

#ifndef CONTINUABLE_PRIMITIVES_HPP_INCLUDED
#define CONTINUABLE_PRIMITIVES_HPP_INCLUDED

#include <continuable/detail/core/types.hpp>
#include <continuable/detail/utility/identity.hpp>

namespace cti {
/// \defgroup Primitives Primitives
/// provides basic tag types for creating a customized callbacks
/// and continuations.
///
/// For the callback and the continuation `Args...` represents the
/// asynchronous result:
/// ```cpp
/// template<typename... Args>
/// struct continuation {
///   void operator() (callback<Args...>);
///   bool operator() (cti::is_ready_arg_t) const;
///   result<Args...> operator() (cti::unpack_arg_t);
/// };
/// ```
/// ```cpp
/// template<typename... Args>
/// struct callback {
///   void operator() (Args...) &&;
///   void operator() (cti::exception_arg_t, cti::exception_t) &&;
/// };
/// ```
/// \{

/// Represents the tag type that is used to specify the signature hint
/// of a continuable_base or promise_base.
///
/// \since 4.0.0
template <typename... Args>
using signature_arg_t = detail::identity<Args...>;

/// Represents the tag type that is used to query the continuation
/// for whether it resolves the callback instantly with its arguments
/// without having side effects.
///
/// \since 4.0.0
struct is_ready_arg_t {};

/// Represents the tag type that is used to unpack the result of a continuation.
///
/// \attention It's required that the query of is_ready_arg_t returns true,
///            otherwise the behaviour when unpacking is unspecified.
///
/// \since 4.0.0
struct unpack_arg_t {};

/// \copydoc unpack_arg_t
///
/// \deprecated The query_arg_t was deprecated because of
///             its new naming unpack_arg_t.
///
[[deprecated("The dispatch_error_tag was replaced by unpack_arg_t and will "
             "be removed in a later major version!")]] //
    typedef unpack_arg_t query_arg_t;

/// Represents the tag type that is used to disambiguate the
/// callback operator() in order to take the exception asynchronous chain.
///
/// \note see continuable::next for details.
///
/// \since 4.0.0
struct exception_arg_t {};

/// \copydoc exception_arg_t
///
/// \deprecated The dispatch_error_tag was deprecated in order to move closer
///             to the types specified in the "A Unified Future" proposal
///             especially regarding naming types similar.
///
[[deprecated("The dispatch_error_tag was replaced by exception_arg_t and will "
             "be removed in a later major version!")]] //
    typedef exception_arg_t dispatch_error_tag;

/// Represents the type that is used as exception type
///
/// By default this type deduces to `std::exception_ptr`.
/// If `CONTINUABLE_WITH_NO_EXCEPTIONS` is defined the type
/// will be a `std::error_condition`.
/// A custom error type may be set through
/// defining `CONTINUABLE_WITH_CUSTOM_ERROR_TYPE`.
///
/// \since 4.0.0
using exception_t = detail::types::exception_t;

/// \copydoc exception_t
///
/// \deprecated The error_type was deprecated in order to move closer
///             to the types specified in the "A Unified Future" proposal
///             especially regarding naming types similar.
///
[[deprecated("The error_type was replaced by exception_t and will "
             "be removed in a later major version!")]] //
    typedef exception_t error_type;

/// Represents the type that is used to disable the special meaning of types
/// which are returned by a asynchronous result handler.
/// See cti::plain for details.
///
/// \since 4.0.0
template <typename T>
using plain_t = detail::types::plain_tag<T>;
/// \}
} // namespace cti

#endif // CONTINUABLE_PRIMITIVES_HPP_INCLUDED
