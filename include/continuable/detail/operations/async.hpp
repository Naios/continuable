
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

#ifndef CONTINUABLE_DETAIL_OPERATIONS_ASYNC_HPP_INCLUDED
#define CONTINUABLE_DETAIL_OPERATIONS_ASYNC_HPP_INCLUDED

#include <continuable/continuable-base.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/utility/identity.hpp>

namespace cti {
namespace detail {
namespace operations {
template <typename Callable, typename... Args>
auto async(Callable&& callable, Args&&... args) {
  using result_t =
      decltype(util::invoke(std::forward<decltype(callable)>(callable),
                            std::forward<decltype(args)>(args)...));

  auto const hint =
      decltype(base::decoration::invoker_of(identify<result_t>{}))::hint();

  auto continuation = [callable = std::forward<decltype(callable)>(callable),
                       args = std::make_tuple(std::forward<decltype(args)>(
                           args)...)](auto&& promise) mutable {
    // Select the invoker
    auto invoker = base::decoration::invoker_of(identify<result_t>{});

    traits::unpack([&](auto&&... args) {
      // Invoke the promise through the dedicated invoker
      invoker(std::move(callable), std::forward<decltype(promise)>(promise),
              std::forward<decltype(args)>(args)...);
    });
  };

  return base::attorney::create_from(std::move(continuation), //
                                     hint, util::ownership{});
}
} // namespace operations

} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_OPERATIONS_ASYNC_HPP_INCLUDED
