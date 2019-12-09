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
#include <asio/version.hpp>

#if (ASIO_VERSION / 100 % 1000) <= 12
#define CTI_DETAIL_ASIO_HAS_NO_INTEGRATION
#elif (ASIO_VERSION / 100 % 1000) <= 14
#define CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION
#endif

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <asio/system_error.hpp>
#endif

#define CTI_DETAIL_ASIO_NAMESPACE_BEGIN namespace asio {
#define CTI_DETAIL_ASIO_NAMESPACE_END }
#else
#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#if (BOOST_VERSION / 100 % 1000) <= 69
#define CTI_DETAIL_ASIO_HAS_NO_INTEGRATION
#elif (BOOST_VERSION / 100 % 1000) <= 71
#define CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION
#endif

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <boost/system/system_error.hpp>
#endif

#define CTI_DETAIL_ASIO_NAMESPACE_BEGIN                                        \
  namespace boost {                                                            \
  namespace asio {
#define CTI_DETAIL_ASIO_NAMESPACE_END                                          \
  }                                                                            \
  }
#endif

#if defined(CTI_DETAIL_ASIO_HAS_NO_INTEGRATION)
#error \
  "First-class ASIO support for continuable requires the form of "\
  "`async_result` with an `initiate` static member function, which was added " \
  "in standalone ASIO 1.13.0 and Boost ASIO 1.70. Older versions can be " \
  "integrated manually with `cti::promisify`."
#endif

#include <utility>

namespace cti {
namespace detail {
namespace asio {

#if defined(ASIO_STANDALONE)
using error_code_t = ::asio::error_code;
using error_cond_t = std::error_condition;

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
using exception_t = ::asio::system_error;
#endif
#else
using error_code_t = ::boost::system::error_code;
using error_cond_t = ::boost::system::error_condition;

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
using exception_t = ::boost::system::system_error;
#endif
#endif

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
template <typename Promise>
void promise_exception_helper(Promise& promise, error_code_t e) noexcept {
  promise.set_exception(std::make_exception_ptr(exception_t(std::move(e))));
}
#else
template <typename Promise>
void promise_exception_helper(Promise& promise, error_code_t e) noexcept
    ->decltype(promise.set_exception(std::move(e))) {
  return promise.set_exception(std::move(e));
}

template <typename Promise>
void promise_exception_helper(Promise& promise, error_code_t e) noexcept
    ->decltype(promise.set_exception(error_cond_t(e.value(), e.category()))) {
  promise.set_exception(error_cond_t(e.value(), e.category()));
}

#endif

// Binds `promise` to the first argument of a continuable resolver, giving it
// the signature of an ASIO handler.
template <typename Promise>
auto promise_resolver_handler(Promise&& promise) noexcept {
  return [promise = std::forward<Promise>(promise)](
             error_code_t e, auto&&... args) mutable noexcept {
    if (e) {
      promise_exception_helper(promise, std::move(e));
    } else {
      promise.set_value(std::forward<decltype(args)>(args)...);
    }
  };
}

// Helper struct wrapping a call to `cti::make_continuable` and, if needed,
// providing an erased, explicit `return_type` for `async_result`.
template <typename Signature>
struct initiate_make_continuable;

template <typename... Args>
struct initiate_make_continuable<void(error_code_t, Args...)> {
  using is_void_cti_t = std::integral_constant<bool, sizeof...(Args) == 0>;

#if defined(CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION)
  using erased_return_type =
      std::conditional_t<is_void_cti_t::value, cti::continuable<>,
                         cti::continuable<Args...>>;
#endif

  template <typename Continuation, typename IsVoid = is_void_cti_t>
  auto operator()(Continuation&& continuation,
                  std::enable_if_t<IsVoid::value>* = 0) {
    return cti::make_continuable<void>(
        std::forward<Continuation>(continuation));
  }

  template <typename Continuation, typename IsVoid = is_void_cti_t>
  auto operator()(Continuation&& continuation,
                  std::enable_if_t<!IsVoid::value>* = 0) {
    return cti::make_continuable<Args...>(
        std::forward<Continuation>(continuation));
  }
};

template <typename... Args>
struct initiate_make_continuable<void(error_code_t const&, Args...)>
    : initiate_make_continuable<void(error_code_t, Args...)> {};

} // namespace asio
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED
