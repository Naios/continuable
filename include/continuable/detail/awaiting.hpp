
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

namespace cti {
namespace detail {
namespace awaiting {
/// We import the coroutine handle in our namespace
using std::experimental::coroutine_handle;

/// An object which provides the internal buffer and helper methods
/// for waiting on a continuable in a stackless coroutine.
template <typename Continuable>
class awaitable {
  /// The continuable which is invoked upon suspension
  Continuable continuable_;
  /// A cache which is used to pass the result of the continuation
  /// to the
  util::expected<int /*TODO*/> cache_;

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
    std::move(continuable_).flow([h, this](auto&&... args) {
      resolve();
      h.resume();
    });
  }

  void await_resume() {
    // if ec throw
    // return n;
    // return
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
