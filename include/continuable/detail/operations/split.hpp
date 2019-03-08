
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

#ifndef CONTINUABLE_DETAIL_OPERATIONS_SPLIT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_OPERATIONS_SPLIT_HPP_INCLUDED

#include <tuple>
#include <utility>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-traverse.hpp>
#include <continuable/continuable-types.hpp>

namespace cti {
namespace detail {
namespace operations {
template <typename First, typename... Promises>
class split_promise {
  First first_;
  std::tuple<Promises...> promises_;

public:
  explicit split_promise(First first, Promises... promises)
      : first_(std::move(first)), promises_(std::move(promises)...) {
  }

  template <typename... Args>
  void operator()(Args... args) && {
    traverse_pack(
        [&](auto&& promise) mutable -> void {
          if (promise) {
            std::forward<decltype(promise)>(promise)(args...);
          }
        },
        std::move(promises_));

    if (first_) {
      std::move(first_)(std::forward<Args>(args)...);
    }
  }

  void operator()(exception_arg_t tag, exception_t exception) && {
    traverse_pack(
        [&](auto&& promise) mutable -> void {
          if (promise) {
            std::forward<decltype(promise)>(promise)(tag, exception);
          }
        },
        std::move(promises_));

    if (first_) {
      std::move(first_)(tag, std::move(exception));
    }
  }

  template <typename... Args>
  void set_value(Args... args) {
    std::move (*this)(std::move(args)...);
  }

  void set_exception(exception_t error) {
    std::move (*this)(exception_arg_t{}, std::move(error));
  }

  explicit operator bool() const noexcept {
    bool is_valid = bool(first_);
    traverse_pack(
        [&](auto&& promise) mutable -> void {
          if (!is_valid && bool(promise)) {
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
