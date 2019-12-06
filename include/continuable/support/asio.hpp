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

#ifndef CONTINUABLE_SUPPORT_ASIO_HPP_INCLUDED
#define CONTINUABLE_SUPPORT_ASIO_HPP_INCLUDED

#include <continuable/continuable-base.hpp>
#include <continuable/detail/support/asio.hpp>
#include <continuable/detail/utility/traits.hpp>

CTI_ASIO_NAMESPACE_BEGIN

// Class used to specify an asynchronous operation should return a continuable.
struct use_cti_t {};

// Special value for instance of `use_cti_t`.
constexpr use_cti_t use_cti{};

template <typename Signature>
class async_result<use_cti_t, Signature> {
public:
  template <typename Initiation, typename... Args>
  static auto initiate(Initiation initiation, use_cti_t, Args... args) {
    return cti::make_continuable<
        cti::detail::asio::continuable_result_t<Signature>>(
        [initiation = std::move(initiation),
         init_args =
             std::make_tuple(std::move(args)...)](auto&& promise) mutable {
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

CTI_ASIO_NAMESPACE_END

#endif // CONTINUABLE_SUPPORT_ASIO_HPP_INCLUDED
