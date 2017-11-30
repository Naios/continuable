
/*

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

#ifndef CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_AWAITING_HPP_INCLUDED__

// Exlude this header when coroutines are not available
#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

#include <experimental/coroutine>

#include <continuable/continuable-api.hpp>
#include <continuable/detail/expected.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
namespace detail {
namespace awaiting {
namespace detail {
template <typename T>
struct result_trait {
  using expected = util::expected<T>;

  static auto wrap(T arg) {
    return std::move(arg);
  }
};
template <typename... T>
struct result_trait<traits::identity<T...>> {
  using expected = util::expected<std::tuple<T...>>;

  static auto wrap(T... args) {
    return std::make_tuple(std::move(args)...);
  }
};

template <typename Continuable>
using result_storage_of_t =
    result_trait<decltype(hint_of(traits::identity_of<Continuable>()))>;
} // namespace detail

/// We import the coroutine handle in our namespace
using std::experimental::coroutine_handle;

/// An object which provides the internal buffer and helper methods
/// for waiting on a continuable in a stackless coroutine.
template <typename Continuable>
class awaitable {
  using trait_t = detail::result_trait<Continuable>;

  /// The continuable which is invoked upon suspension
  Continuable continuable_;
  /// A cache which is used to pass the result of the continuation
  /// to the coroutine.
  typename trait_t::expected result_;

public:
  /// Since continuables are evaluated lazily we are not
  /// capable to say whether the resumption will be instantly.
  bool await_ready() const noexcept {
    return false;
  }

  /// Suspend the current context
  // TODO Convert this to an r-value function once possible
  void await_suspend(coroutine_handle<> h) {
    // Forward every result to the current awaitable
    std::move(continuable_)
        .next([h, this](auto&&... args) {
          resolve(std::forward<decltype(args)>(args)...);
          h.resume();
        })
        .done();
  }

  void await_resume() {
    if (result_) {
      return result_.get_value();
    } else {
      throw result_.get_error();
    }
  }

private:
  /// Resolve the continuation through the result
  template <typename... Args>
  void resolve(Args&&... args) {
    // ...
  }

  /// Resolve the continuation through an error
  void resolve(types::dispatch_error_tag, types::error_type error) {
    // ...
  }
};

/// Converts a continuable into an awaitable object as described by
/// the C++ coroutine TS.
template <typename T>
auto create_awaiter(T&& continuable) {
  return awaitable<std::decay_t<T>>{std::forward<T>(continuable)};
}
} // namespace awaiting
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
#endif // CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED__
