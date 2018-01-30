
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

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

#ifndef CONTINUABLE_PROMISE_BASE_HPP_INCLUDED__
#define CONTINUABLE_PROMISE_BASE_HPP_INCLUDED__

#include <type_traits>
#include <utility>

#include <continuable/continuable-api.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
/// The promise_base makes it possible to resolve an asynchronous
/// continuable through it's result or through an error type.
///
/// Use the promise type defined in `continuable/continuable_types.hpp`,
/// in order to use this class.
///
/// \since version 2.0.0
// clang-format off
template <typename Data, typename Hint>
class promise_base
  /// \cond false
  ;
template <typename Data, typename... Args>
class promise_base<Data, detail::hints::signature_hint_tag<Args...>>
    : detail::util::non_copyable
  /// \endcond
{ // clang-format on

  /// \cond false
  // The callback type
  Data data_;
  /// \endcond

public:
  /// Constructor accepting the data object
  explicit promise_base(Data data) : data_(std::move(data)) {
  }

  /// Constructor accepting any object convertible to the data object
  template <typename OData, std::enable_if_t<std::is_convertible<
                                std::decay_t<OData>, Data>::value>* = nullptr>
  promise_base(OData&& data) : data_(std::forward<OData>(data)) {
  }

  /// Resolves the continuation with the given values
  ///
  /// \since version 2.0.0
  void operator()(Args... args) && {
    std::move(data_)(std::move(args)...);
  }
  /// Resolves the continuation with the given exception
  ///
  /// \since version 2.0.0
  void operator()(detail::types::dispatch_error_tag tag,
                  detail::types::error_type exception) && {
    std::move(data_)(tag, std::move(exception));
  }

  /// Resolves the continuation with the given values
  ///
  /// \since version 2.0.0
  void set_value(Args... args) {
    std::move(data_)(std::move(args)...);
  }

  /// Resolves the continuation with the given exception
  ///
  /// \since version 2.0.0
  void set_exception(detail::types::error_type exception) {
    std::move(data_)(detail::types::dispatch_error_tag{}, std::move(exception));
  }
};
} // namespace cti

#endif // CONTINUABLE_PROMISE_BASE_HPP_INCLUDED__
