
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_PROMISIFY_HPP_INCLUDED
#define CONTINUABLE_PROMISIFY_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <continuable/detail/promisify.hpp>

namespace cti {
/// \defgroup Promisify Promisify
/// provides helper methods to convert various callback styles to
/// \link continuable_base continuable_bases\endlink.
/// \{

/// Helper class for converting callback taking callable types into a
/// a continuable. Various styles are supported.
/// - `from`: Converts callback taking callable types into continuables
///           which pass an error code as first parameter and the rest of
///           the result afterwards.
///
/// \tparam Result The result of the converted continuable, this should align
///                with the arguments that are passed to the callback.
///
/// \since         3.0.0
template <typename... Result>
class promisify {
  using helper = detail::convert::promisify_helper<Result...>;

public:
  /// Converts callback taking callable types into a continuable.
  /// This applies to calls which pass an error code as first parameter
  /// and the rest of the asynchronous result afterwards.
  ///
  /// See an example of how to promisify boost asio's async_resolve below:
  /// ```cpp
  /// auto async_resolve(std::string host, std::string service) {
  ///   return cti::promisify<asio::ip::udp::resolver::iterator>::from(
  ///       [&](auto&&... args) {
  ///         resolver_.async_resolve(std::forward<decltype(args)>(args)...);
  ///       },
  ///       std::move(host), std::move(service));
  /// }
  /// ```
  ///
  /// If the error code which is passed as first parameter is set there are
  /// two behaviours depending whether exceptions are enabled:
  /// - If exceptions are enabled the error type is passed via
  ///   an exception_ptr to the failure handler.
  /// - If exceptions are disabled the error type is converted to a
  ///   `std::error_conditon` and passed down to the error handler.
  ///
  /// \since  3.0.0
  template <typename Callable, typename... Args>
  static auto from(Callable&& callable, Args&&... args) {
    return helper::template from<detail::convert::promisify_default>(
        std::forward<Callable>(callable), std::forward<Args>(args)...);
  }
};
/// \}
} // namespace cti

#endif // CONTINUABLE_PROMISIFY_HPP_INCLUDED
