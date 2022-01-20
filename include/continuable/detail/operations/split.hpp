
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

#ifndef CONTINUABLE_DETAIL_OPERATIONS_SPLIT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_OPERATIONS_SPLIT_HPP_INCLUDED

#include <tuple>
#include <utility>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-traverse.hpp>
#include <continuable/continuable-types.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace operations {
template <typename T, bool Else, typename = void>
struct operator_bool_or {
  template <typename O>
  static bool get(O&& /*obj*/) noexcept {
    return Else;
  }
};
template <typename T, bool Else>
struct operator_bool_or<T, Else,
                        traits::void_t<decltype(bool(std::declval<T&>()))>> {
  template <typename O>
  static bool get(O&& obj) noexcept {
    return bool(obj);
  }
};

template <typename First, typename... Promises>
class split_promise {
  First first_;
  std::tuple<Promises...> promises_;

public:
  explicit split_promise(First first, Promises... promises)
      : first_(std::move(first)), promises_(std::move(promises)...) {
  }

  template <typename... Args>
  void operator()(Args&&... args) && {
    traverse_pack(
        [&](auto&& promise) mutable -> void {
          using accessor =
              operator_bool_or<traits::unrefcv_t<decltype(promise)>, true>;
          if (accessor::get(promise)) {
            std::forward<decltype(promise)>(promise)(args...);
          }
        },
        std::move(promises_));

    if (operator_bool_or<First, true>::get(first_)) {
      std::move(first_)(std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void set_value(Args... args) noexcept {
    std::move (*this)(std::move(args)...);
  }

  void set_exception(exception_t error) noexcept {
    std::move (*this)(exception_arg_t{}, std::move(error));
  }

  void set_canceled() noexcept {
    std::move (*this)(exception_arg_t{}, exception_t{});
  }

  explicit operator bool() const noexcept {
    bool is_valid = operator_bool_or<First, true>::get(first_);
    traverse_pack(
        [&](auto&& promise) mutable -> void {
          using accessor =
              operator_bool_or<traits::unrefcv_t<decltype(promise)>, true>;
          if (!is_valid && accessor::get(promise)) {
            is_valid = true;
          }
        },
        promises_);
    return is_valid;
  }
};
} // namespace operations
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_OPERATIONS_SPLIT_HPP_INCLUDED
