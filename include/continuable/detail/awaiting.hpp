
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
#include <continuable/detail/features.hpp>

namespace cti {
namespace detail {
namespace awaiting {
using std::experimental::coroutine_handle;
/// Converts a continuable into an awaitable object as described by
/// the C++ coroutine TS.
template <typename T>
auto create_awaiter(T&& continuable) {
  struct Awaiter {
    bool is_ready() const noexcept {
      return false;
    }

    void await_suspend(coroutine_handle<> h) && {

      h.resume();
    }

    auto await_resume() {
      // if ec throw
      // return n;
      // return
    }
  };

  return Awaiter();
}
} // namespace awaiting
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
#endif // CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED__
