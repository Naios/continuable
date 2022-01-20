
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

#ifndef CONTINUABLE_PROMISE_BASE_HPP_INCLUDED
#define CONTINUABLE_PROMISE_BASE_HPP_INCLUDED

#include <cassert>
#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
/// \defgroup Base Base
/// provides classes and functions to create continuable_base objects.
/// \{

/// The promise_base makes it possible to resolve an asynchronous
/// continuable through it's result or through an error type.
///
/// Use the promise type defined in `continuable/continuable_types.hpp`,
/// in order to use this class.
///
/// If we want to resolve the  promise_base trough the call operator,
/// and we want to resolve it through an exception, we must call it with a
/// exception_arg_t as first and the exception as second argument.
/// Additionally the promise is resolveable only through its call
/// operator when invoked as an r-value.
///
/// \since 2.0.0
// clang-format off
template <typename Data, typename Hint>
class promise_base
  /// \cond false
  ;
template <typename Data, typename... Args>
class promise_base<Data, detail::identity<Args...>>
    : detail::util::non_copyable
  /// \endcond
{ // clang-format on

  /// \cond false
  // The callback type
  Data data_;
  /// \endcond

public:
  /// Constructor for constructing an empty promise
  explicit promise_base() = default;
  /// Constructor accepting the data object
  explicit promise_base(Data data) : data_(std::move(data)) {
  }

  /// \cond false
  promise_base(promise_base&&) = default;
  promise_base(promise_base const&) = delete;

  promise_base& operator=(promise_base&&) = default;
  promise_base& operator=(promise_base const&) = delete;
  /// \endcond

  /// Constructor accepting any object convertible to the data object
  template <typename OData,
            std::enable_if_t<std::is_convertible<
                detail::traits::unrefcv_t<OData>, Data>::value>* = nullptr>
  /* implicit */ promise_base(OData&& data) : data_(std::forward<OData>(data)) {
  }

  /// Assignment operator accepting any object convertible to the data object
  template <typename OData,
            std::enable_if_t<std::is_convertible<
                detail::traits::unrefcv_t<OData>, Data>::value>* = nullptr>
  promise_base& operator=(OData&& data) {
    data_ = std::forward<OData>(data);
    return *this;
  }

  /// Resolves the continuation with the given values.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the promise is valid operator bool() returns true.
  ///            Calling this method will invalidate the promise such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in promise_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid promise_base is undefined!
  ///
  /// \since  2.0.0
  void operator()(Args... args) && noexcept {
    assert(data_);
    std::move(data_)(std::move(args)...);
    data_ = nullptr;
  }
  /// Resolves the continuation with the given exception.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the promise is valid operator bool() returns true.
  ///            Calling this method will invalidate the promise such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in promise_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid promise_base is undefined!
  ///
  /// \since  2.0.0
  void operator()(exception_arg_t tag, exception_t exception) && noexcept {
    assert(data_);
    std::move(data_)(tag, std::move(exception));
    data_ = nullptr;
  }

  /// Resolves the continuation with the given values.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the promise is valid operator bool() returns true.
  ///            Calling this method will invalidate the promise such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in promise_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid promise_base is undefined!
  ///
  /// \since  2.0.0
  void set_value(Args... args) noexcept {
    // assert(data_);
    std::move(data_)(std::move(args)...);
    data_ = nullptr;
  }

  /// Resolves the continuation with the given exception.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the promise is valid operator bool() returns true.
  ///            Calling this method will invalidate the promise such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in promise_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid promise_base is undefined!
  ///
  /// \since  2.0.0
  void set_exception(exception_t exception) noexcept {
    assert(data_);
    std::move(data_)(exception_arg_t{}, std::move(exception));
    data_ = nullptr;
  }

  /// Resolves the continuation with the cancellation token which is represented
  /// by a default constructed exception_t.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the promise is valid operator bool() returns true.
  ///            Calling this method will invalidate the promise such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in promise_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid promise_base is undefined!
  ///
  /// \since  4.0.0
  void set_canceled() noexcept {
    assert(data_);
    std::move(data_)(exception_arg_t{}, exception_t{});
    data_ = nullptr;
  }

  /// Returns true if the continuation is valid (non empty).
  ///
  /// \throws This method never throws an exception.
  ///
  /// \since  4.0.0
  explicit operator bool() const noexcept {
    return bool(data_);
  }
};
/// \}
} // namespace cti

#endif // CONTINUABLE_PROMISE_BASE_HPP_INCLUDED
