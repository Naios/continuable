
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

  Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_WORK_BASE_HPP_INCLUDED
#define CONTINUABLE_WORK_BASE_HPP_INCLUDED

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

/// The work_base makes it possible to resolve an asynchronous
/// continuable through it's result or through an error type.
///
/// Use the work type defined in `continuable/continuable_types.hpp`,
/// in order to use this class.
///
/// If we want to resolve the  work_base trough the call operator,
/// and we want to resolve it through an exception, we must call it with a
/// exception_arg_t as first and the exception as second argument.
/// Additionally the work is resolveable only through its call
/// operator when invoked as an r-value.
///
/// \since 4.0.0
template <typename Data>
class work_base
    /// \cond false
    : detail::util::non_copyable
/// \endcond
{ // clang-format on

  /// \cond false
  // The work type
  Data data_;
  /// \endcond

public:
  /// Constructor for constructing an empty work
  explicit work_base() = default;
  /// Constructor accepting the data object
  explicit work_base(Data data) : data_(std::move(data)) {
  }

  /// \cond false
  work_base(work_base&&) = default;
  work_base(work_base const&) = delete;

  work_base& operator=(work_base&&) = default;
  work_base& operator=(work_base const&) = delete;
  /// \endcond

  /// Constructor accepting any object convertible to the data object
  template <typename OData,
            std::enable_if_t<std::is_convertible<
                detail::traits::unrefcv_t<OData>, Data>::value>* = nullptr>
  /* implicit */ work_base(OData&& data) : data_(std::forward<OData>(data)) {
  }

  /// Assignment operator accepting any object convertible to the data object
  template <typename OData,
            std::enable_if_t<std::is_convertible<
                detail::traits::unrefcv_t<OData>, Data>::value>* = nullptr>
  work_base& operator=(OData&& data) {
    data_ = std::forward<OData>(data);
    return *this;
  }

  /// Resolves the continuation with the given values.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the work is valid operator bool() returns true.
  ///            Calling this method will invalidate the work such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in work_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid work_base is undefined!
  ///
  /// \since  4.0.0
  void operator()() && noexcept {
    std::move(data_)();
    data_ = nullptr;
  }
  /// Resolves the continuation with the given exception.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the work is valid operator bool() returns true.
  ///            Calling this method will invalidate the work such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in work_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid work_base is undefined!
  ///
  /// \since  4.0.0
  void operator()(exception_arg_t tag, exception_t exception) && noexcept {
    std::move(data_)(tag, std::move(exception));
    data_ = nullptr;
  }

  /// Resolves the continuation with the given values.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the work is valid operator bool() returns true.
  ///            Calling this method will invalidate the work such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in work_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid work_base is undefined!
  ///
  /// \since  4.0.0
  void set_value() noexcept {
    std::move(data_)();
    data_ = nullptr;
  }

  /// Resolves the continuation with the given exception.
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once,
  ///            when the work is valid operator bool() returns true.
  ///            Calling this method will invalidate the work such that
  ///            subsequent calls to operator bool() will return false.
  ///            This behaviour is only consistent in work_base and
  ///            non type erased promises may behave differently.
  ///            Invoking an invalid work_base is undefined!
  ///
  /// \since  4.0.0
  void set_exception(exception_t exception) noexcept {
    std::move(data_)(exception_arg_t{}, std::move(exception));
    data_ = nullptr;
  }
};
/// \}
} // namespace cti

#endif // CONTINUABLE_WORK_BASE_HPP_INCLUDED
