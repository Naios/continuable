/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.1.0

  Copyright(c) 2015 - 2020 Denis Blank <denis.blank at outlook dot com>

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
/// \since 4.0.0
struct use_continuable_t {};

/// Special value for instance of `asio_token_t`
///
/// \copydetails use_continuable_t
constexpr use_continuable_t use_continuable{};
} // namespace cti

CTI_DETAIL_ASIO_NAMESPACE_BEGIN

template <typename Signature>
class async_result<cti::use_continuable_t, Signature> {
public:
#if defined(CTI_DETAIL_ASIO_HAS_EXPLICIT_RET_TYPE_INTEGRATION)
  using return_type = typename cti::detail::asio::initiate_make_continuable<
      Signature>::erased_return_type;
#endif

  template <typename Initiation, typename... Args>
  static auto initiate(Initiation initiation, cti::use_continuable_t,
                       Args... args) {
    return cti::detail::asio::initiate_make_continuable<Signature>{}(
        [initiation = std::move(initiation),
         init_args = std::make_tuple(std::move(args)...)](
            auto&& promise) mutable {
          cti::detail::traits::unpack(
              [initiation = std::move(initiation),
               handler = cti::detail::asio::promise_resolver_handler(
                   std::forward<decltype(promise)>(promise))](
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
