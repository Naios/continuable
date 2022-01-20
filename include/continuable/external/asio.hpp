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

#ifndef CONTINUABLE_EXTERNAL_ASIO_HPP_INCLUDED
#define CONTINUABLE_EXTERNAL_ASIO_HPP_INCLUDED

#include <continuable/continuable-base.hpp>
#include <continuable/detail/external/asio.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
/// The error code type used by your asio distribution
///
/// \since 4.1.0
using asio_error_code_t = detail::asio::error_code_t;

/// The basic error code enum used by your asio distribution
///
/// \since 4.1.0
using asio_basic_errors_t = detail::asio::basic_errors_t;

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
/// The system error type used by your asio distribution
///
/// \since 4.1.0
using asio_system_error_t = detail::asio::system_error_t;
#endif // CONTINUABLE_HAS_EXCEPTIONS

/// Type used as an ASIO completion token to specify an asynchronous operation
/// that should return a continuable_base.
///
/// - Boost 1.70 or asio 1.13.0 is required for the async initiation
/// - Until boost 1.72 or asio 1.16.0 overhead through an additional type
///   erasure is added. It is recommended to update to those versions.
///
/// The special static variable use_continuable can be appended to any
/// (boost) asio function that accepts a callback to make it return a
/// continuable_base.
///
/// ```cpp
/// #include <continuable/continuable.hpp>
/// #include <continuable/external/asio.hpp>
/// #include <asio.hpp>
///
/// // ...
///
/// asio::tcp::resolver resolver(...);
/// resolver.async_resolve("127.0.0.1", "daytime", cti::use_continuable)
///   .then([](asio::udp::resolver::iterator iterator) {
///     // ...
///   });
/// ```
///
/// \tparam Mapper The token can be instantiated with a custom mapper
///         for asio error codes which makes it possible to ignore
///         errors or treat them as cancellation types.
///         The mapper has the following form:
/// ```
/// struct my_mapper {
///   constexpr my_mapper() noexcept {}
///
///   /// Returns true when the error_code_t is a type which represents
///   /// cancellation and
///   bool is_cancellation(error_code_t const& /*ec*/) const noexcept {
///     return false;
///   }
///   bool is_ignored(error_code_t const& /*ec*/) const noexcept {
///     return false;
///   }
/// };
/// ```
///
/// \attention `asio::error::basic_errors::operation_aborted` errors returned
///            by asio are automatically transformed into a default constructed
///            exception type which represents "operation canceled" by the
///            user or program. If you intend to retrieve the full
///            asio::error_code without remapping use the use_continuable_raw_t
///            completion token instead!
///
/// \since 4.0.0
template <typename Mapper = detail::asio::map_default>
struct use_continuable_t : public Mapper {
  using Mapper::Mapper;
};

/// \copydoc use_continuable_t
///
/// The raw async completion handler token does not remap the asio error
/// `asio::error::basic_errors::operation_aborted` to a default constructed
/// exception type.
///
/// \since 4.1.0
using use_continuable_raw_t = use_continuable_t<detail::asio::map_none>;

/// Special value for instance of use_continuable_t which performs remapping
/// of asio error codes to align the cancellation behaviour with the library.
///
/// \copydetails use_continuable_t
constexpr use_continuable_t<> use_continuable{};

/// Special value for instance of use_continuable_raw_t which doesn't perform
/// remapping of asio error codes and rethrows the raw error code.
///
/// \copydetails use_continuable_raw_t
constexpr use_continuable_raw_t use_continuable_raw{};

/// Represents a special asio completion token which treats the given
/// asio basic error codes as success instead of failure.
///
/// `asio::error::basic_errors::operation_aborted` is mapped
/// as cancellation token.
///
/// \since 4.1.0
template <typename... Args>
auto use_continuable_ignoring(Args&&... args) noexcept {
  return use_continuable_t<detail::asio::map_ignore<sizeof...(Args)>>{
      {asio_basic_errors_t(std::forward<Args>(args))...}};
}
} // namespace cti

CTI_DETAIL_ASIO_NAMESPACE_BEGIN

template <typename Signature, typename Matcher>
class async_result<cti::use_continuable_t<Matcher>, Signature> {
public:
#if defined(CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION)
  using return_type = typename cti::detail::asio::initiate_make_continuable<
      Signature>::erased_return_type;
#endif

  template <typename Initiation, typename... Args>
  static auto initiate(Initiation initiation,
                       cti::use_continuable_t<Matcher> token, Args... args) {
    return cti::detail::asio::initiate_make_continuable<Signature>{}(
        [initiation = std::move(initiation), token = std::move(token),
         init_args = std::make_tuple(std::move(args)...)](
            auto&& promise) mutable {
          cti::detail::traits::unpack(
              [initiation = std::move(initiation),
               handler = cti::detail::asio::promise_resolver_handler(
                   std::forward<decltype(promise)>(promise), std::move(token))](
                  auto&&... args) mutable {
                std::move(initiation)(std::move(handler),
                                      std::forward<decltype(args)>(args)...);
              },
              std::move(init_args));
        });
  }
};

CTI_DETAIL_ASIO_NAMESPACE_END

#undef CTI_DETAIL_ASIO_NAMESPACE_BEGIN
#undef CTI_DETAIL_ASIO_NAMESPACE_END
#undef CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION

#endif // CONTINUABLE_EXTERNAL_ASIO_HPP_INCLUDED
