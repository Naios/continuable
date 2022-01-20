
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

#ifndef CONTINUABLE_DETAIL_RANGE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_RANGE_HPP_INCLUDED

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace range {
/// Deduces to a true_type if the given type is an interator
template <typename T, typename = void>
struct is_iterator : std::false_type {};
template <typename T>
struct is_iterator<T,
                   traits::void_t<typename std::iterator_traits<T>::value_type>>
    : std::true_type {};

/// Moves the content of the given iterators to a persistent storage
template <typename Iterator>
auto persist_range(Iterator begin, Iterator end) {
  std::vector<typename std::iterator_traits<Iterator>::value_type> storage;
  // TODO Find out why the superior idiom below has issues with move only types:
  // storage.insert(storage.end(), std::make_move_iterator(begin),
  //                std::make_move_iterator(end));
  std::move(begin, end, std::back_inserter(storage));
  return storage;
}
} // namespace range
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_RANGE_HPP_INCLUDED
