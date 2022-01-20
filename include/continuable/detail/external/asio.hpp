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

#ifndef CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED

#include <array>
#include <utility>
#include <continuable/continuable-base.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/features.hpp>

#if defined(ASIO_STANDALONE)
#  include <asio/async_result.hpp>
#  include <asio/error.hpp>
#  include <asio/error_code.hpp>
#  include <asio/version.hpp>

#  if defined(CONTINUABLE_HAS_EXCEPTIONS)
#    include <asio/system_error.hpp>
#  endif

#  if (ASIO_VERSION < 101300) // 1.13.0
#    define CTI_DETAIL_ASIO_HAS_NO_INTEGRATION
#  elif (ASIO_VERSION < 101600) // 1.16.0 (boost 1.72 baseline)
#    define CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION
#  endif

#  define CTI_DETAIL_ASIO_NAMESPACE_BEGIN namespace asio {
#  define CTI_DETAIL_ASIO_NAMESPACE_END }
#else
#  include <boost/asio/async_result.hpp>
#  include <boost/asio/error.hpp>
#  include <boost/system/error_code.hpp>
#  include <boost/version.hpp>

#  if defined(CONTINUABLE_HAS_EXCEPTIONS)
#    include <boost/system/system_error.hpp>
#  endif

#  if (BOOST_VERSION < 107000) // 1.70
#    define CTI_DETAIL_ASIO_HAS_NO_INTEGRATION
#  elif (BOOST_VERSION < 107200) // 1.72
#    define CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION
#  endif

#  define CTI_DETAIL_ASIO_NAMESPACE_BEGIN                                      \
    namespace boost {                                                          \
    namespace asio {
#  define CTI_DETAIL_ASIO_NAMESPACE_END                                        \
    }                                                                          \
    }
#endif

#if defined(CTI_DETAIL_ASIO_HAS_NO_INTEGRATION)
#  error "First-class ASIO support for continuable requires the form of "\
  "`async_result` with an `initiate` static member function, which was added " \
  "in standalone ASIO 1.13.0 and Boost ASIO 1.70. Older versions can be " \
  "integrated manually with `cti::promisify`."
#endif

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#  include <exception>
#endif

namespace cti {
namespace detail {
namespace asio {

#if defined(ASIO_STANDALONE)
using error_code_t = ::asio::error_code;
using basic_errors_t = ::asio::error::basic_errors;

#  if defined(CONTINUABLE_HAS_EXCEPTIONS)
using system_error_t = ::asio::system_error;
#  endif
#else
using error_code_t = ::boost::system::error_code;
using basic_errors_t = ::boost::asio::error::basic_errors;

#  if defined(CONTINUABLE_HAS_EXCEPTIONS)
using system_error_t = ::boost::system::system_error;
#  endif
#endif

template <typename Promise, typename Token>
class promise_resolver {
public:
  explicit promise_resolver(Promise promise, Token token)
    : promise_(std::move(promise))
    , token_(std::move(token)) {}

  template <typename... T>
  void operator()(T&&... args) noexcept {
    promise_.set_value(std::forward<T>(args)...);
  }

  template <typename... T>
  void operator()(error_code_t e, T&&... args) noexcept {
    if (e) {
      if (!token_.is_ignored(e)) {
        if (token_.is_cancellation(e)) {
          promise_.set_canceled();
          return;
        } else {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
          promise_.set_exception(
              std::make_exception_ptr(system_error_t(std::move(e))));
#else
          promise_.set_exception(exception_t(e.value(), e.category()));
#endif
          return;
        }
      }
    }
    promise_.set_value(std::forward<T>(args)...);
  }

private:
  Promise promise_;
  Token token_;
};

// Binds `promise` to the first argument of a continuable resolver, giving it
// the signature of an ASIO handler.
template <typename Promise, typename Token>
auto promise_resolver_handler(Promise&& promise, Token&& token) noexcept {
  return promise_resolver<std::decay_t<Promise>, std::decay_t<Token>>(
      std::forward<Promise>(promise), std::forward<Token>(token));
}

// Helper struct wrapping a call to `cti::make_continuable` and, if needed,
// providing an erased, explicit `return_type` for `async_result`.
template <typename Signature>
struct initiate_make_continuable;

template <typename... Args>
struct initiate_make_continuable<void(Args...)> {
#if defined(CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION)
  using erased_return_type = continuable<Args...>;
#endif

  template <typename Continuation>
  auto operator()(Continuation&& continuation) {
    return base::attorney::create_from(std::forward<Continuation>(continuation),
                                       identity<Args...>{}, util::ownership{});
  }
};

template <typename... Args>
struct initiate_make_continuable<void(error_code_t, Args...)> {
#if defined(CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION)
  using erased_return_type = continuable<Args...>;
#endif

  template <typename Continuation>
  auto operator()(Continuation&& continuation) {
    return base::attorney::create_from(std::forward<Continuation>(continuation),
                                       identity<Args...>{}, util::ownership{});
  }
};

template <typename... Args>
struct initiate_make_continuable<void(error_code_t const&, Args...)>
  : initiate_make_continuable<void(error_code_t, Args...)> {};

struct map_default {
  constexpr map_default() noexcept {}

  bool is_cancellation(error_code_t const& ec) const noexcept {
    // Continuable uses a default constructed exception type to signal
    // cancellation to the followed asynchronous control flow.
    return ec == basic_errors_t::operation_aborted;
  }
  bool is_ignored(error_code_t const& /*ec*/) const noexcept {
    return false;
  }
};

struct map_none {
  constexpr map_none() noexcept {}

  bool is_cancellation(error_code_t const& /*ec*/) const noexcept {
    return false;
  }
  bool is_ignored(error_code_t const& /*ec*/) const noexcept {
    return false;
  }
};

template <std::size_t Size>
class map_ignore {
public:
  map_ignore(std::array<basic_errors_t, Size> ignored) noexcept
    : ignored_(ignored) {}

  bool is_cancellation(error_code_t const& ec) const noexcept {
    return ec == basic_errors_t::operation_aborted;
  }
  bool is_ignored(error_code_t const& ec) const noexcept {
    for (basic_errors_t ignored : ignored_) {
      if (ec == ignored) {
        return true;
      }
    }
    return false;
  }

private:
  std::array<basic_errors_t, Size> ignored_;
};
} // namespace asio
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ASIO_HPP_INCLUDED
