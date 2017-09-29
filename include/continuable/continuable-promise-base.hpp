
/**

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2017 Denis Blank <denis.blank at outlook dot com>

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

#include <continuable/detail/api.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/hints.hpp>

namespace cti {
template <typename Hint, typename Data>
class promise_base;
template <typename... Args, typename Data>
class promise_base<detail::hints::signature_hint_tag<Args...>, Data> {
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

  /// \cond false
  promise_base(promise_base&&) = default;
  promise_base(promise_base const&) = default;

  promise_base& operator=(promise_base&&) = default;
  promise_base& operator=(promise_base const&) = default;
  /// \endcond

  /// Resolves the continuation with the given values
  void set_value(Args... args) {
    data_(std::move(args)...);
  }

  /// Resolves the continuation with the given values
  void operator()(Args... args) {
    data_(std::move(args)...);
  }

  /// Resolves the continuation with the given error variable.
  void set_error(detail::base::error_type error) {
    data_(detail::base::dispatch_error_tag{}, std::move(error));
  }
};
} // namespace cti

#endif // CONTINUABLE_PROMISE_BASE_HPP_INCLUDED__
