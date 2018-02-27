
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

#ifndef CONTINUABLE_COMPOSITIONS_HPP_INCLUDED
#define CONTINUABLE_COMPOSITIONS_HPP_INCLUDED

#include <utility>

#include <continuable/detail/composition-all.hpp>
#include <continuable/detail/composition-any.hpp>
#include <continuable/detail/composition-seq.hpp>
#include <continuable/detail/composition.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/range.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
/// Connects the given continuables with an *all* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator && for details.
///
/// \since 1.1.0
template <typename... Continuables>
auto when_all(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return CONTINUABLE_FOLD_EXPRESSION(
      &&, std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with an *any* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator|| for details.
///
/// \since 1.1.0
template <typename... Continuables>
auto when_any(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return CONTINUABLE_FOLD_EXPRESSION(
      ||, std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with a *seq* logic.
///
/// \param args The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator>> for details.
///
/// \since 1.1.0
template <typename... Args>
auto when_seq(Args&&... args) {
  return detail::composition::apply_composition(
      detail::composition::composition_strategy_seq_tag{},
      std::forward<Args>(args)...);
}

/// bla
///
/// \copydetail when_seq
///
/// \since 3.0.0
template <
    typename Iterator,
    std::enable_if_t<detail::range::is_iterator<Iterator>::value>* = nullptr>
auto when_seq(Iterator begin, Iterator end) {
  return when_seq(detail::range::persist_range(begin, end));
}
} // namespace cti

#endif // CONTINUABLE_COMPOSITIONS_HPP_INCLUDED
