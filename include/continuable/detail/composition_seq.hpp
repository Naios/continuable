
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

#ifndef CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-promise-base.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace composition {
namespace seq {
/// Connects the left and the right continuable to a sequence
///
/// \note This is implemented in an eager way because we would not gain
///       any profit from chaining sequences lazily.
template <typename Left, typename Right>
auto sequential_connect(Left&& left, Right&& right) {
  left.freeze(right.is_frozen());
  right.freeze();

  return std::forward<Left>(left).then([right = std::forward<Right>(right)](
      auto&&... args) mutable {
    return std::move(right).then([previous = std::make_tuple(
                                      std::forward<decltype(args)>(args)...)](
        auto&&... args) mutable {
      return traits::merge(
          std::move(previous),
          std::make_tuple(std::forward<decltype(args)>(args)...));
    });
  });
}
} // namespace seq

/// Finalizes the seq logic of a given composition
template <>
struct composition_finalizer<composition_strategy_seq_tag> {
  /// TODO
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED
