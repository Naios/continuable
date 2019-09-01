
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
/// work on a different execution context than the current one.
///
/// A work compatible object is passed to any executor that is passed to
/// \see continuable_base::then or \see async_on.
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

  /// Invokes the underlying work
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once!
  ///
  /// \since  4.0.0
  void set_value() noexcept {
    std::move(data_)();
    data_ = nullptr;
  }

  /// Passes an exception to the underlying work
  ///
  /// \throws This method never throws an exception.
  ///
  /// \attention This method may only be called once!
  ///
  /// \since  4.0.0
  void set_exception(exception_t exception) noexcept {
    std::move(data_)(exception_arg_t{}, std::move(exception));
    data_ = nullptr;
  }

  /// \copydoc set_value
  void operator()() && noexcept {
    std::move(data_)();
    data_ = nullptr;
  }

  /// \copydoc set_exception
  void operator()(exception_arg_t tag, exception_t exception) && noexcept {
    std::move(data_)(tag, std::move(exception));
    data_ = nullptr;
  }
};
/// \}
} // namespace cti

#endif // CONTINUABLE_WORK_BASE_HPP_INCLUDED
