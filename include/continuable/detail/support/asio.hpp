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

#ifndef CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED

#if defined(ASIO_STANDALONE)
#include <asio/async_result.hpp>
#include <asio/error_code.hpp>
#include <asio/system_error.hpp>

#define CTI_ASIO_NAMESPACE_BEGIN namespace asio {
#define CTI_ASIO_NAMESPACE_END }
#else
#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#define CTI_ASIO_NAMESPACE_BEGIN namespace boost { namespace asio {
#define CTI_ASIO_NAMESPACE_END }}
#endif

#include <utility>

namespace cti {
namespace detail {
namespace asio {

#if defined(ASIO_STANDALONE)
using error_code_t = ::asio::error_code;
using exception_t = ::asio::system_error;
#else
using error_code_t = ::boost::system::error_code;
using exception_t = ::boost::system::system_error;
#endif

// Binds `promise` to the first argument of a continuable resolver, giving it
// the signature of an ASIO handler.
template <typename Promise>
auto promise_resolver_handler(Promise&& promise) noexcept {
  return [promise = std::forward<Promise>(promise)](
      error_code_t e, auto&&... args) mutable noexcept {
    if (e) {
      promise.set_exception(std::make_exception_ptr(exception_t(std::move(e))));
    } else {
      promise.set_value(std::forward<decltype(args)>(args)...);
    }
  };
}

// Type deduction helper for `Result` in `cti::make_continuable<Result>`
template <typename Signature>
struct continuable_result;

template <>
struct continuable_result<void(error_code_t)> {
  using type = void;
};

template <>
struct continuable_result<void(error_code_t const&)>
    : continuable_result<void(error_code_t)> {};

template <typename T>
struct continuable_result<void(error_code_t, T)> {
  using type = T;
};

template <typename T>
struct continuable_result<void(error_code_t const&, T)>
    : continuable_result<void(error_code_t, T)> {};

template <typename Signature>
using continuable_result_t = typename continuable_result<Signature>::type;



} // namespace asio
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED
