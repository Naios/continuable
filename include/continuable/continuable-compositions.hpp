
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
/// Connects the given arguments with an all logic.
/// All continuables contained inside the given nested pack are
/// invoked at once. On completion the final handler is called
/// with the aggregated result of all continuables.
///
/// \param args Arbitrary arguments which are connected.
///             Every type is allowed as arguments, continuables may be
///             contained inside tuple like types (`std::tuple`)
///             or in homogeneous containers such as `std::vector`.
///             Non continuable arguments are preserved and passed
///             to the final result as shown below:
/// ```cpp
/// cti::when_all(
///     cti::make_ready_continuable(0, 1),
///     2, //< See this plain value
///     std::vector<cti::continuable<int>>{cti::make_ready_continuable(3),
///                                        cti::make_ready_continuable(4)},
///     std::make_tuple(std::make_tuple(cti::make_ready_continuable(5))))
///       .then([](int r0, int r1, int r2, std::vector<int> r34,
///                std::tuple<std::tuple<int>> r5) {
///         // ...
///       });
/// ```
///
/// \see        continuable_base::operator&& for details.
///
/// \since      1.1.0
template <typename... Args>
auto when_all(Args&&... args) {
  return detail::composition::apply_composition(
      detail::composition::composition_strategy_all_tag{},
      std::forward<Args>(args)...);
}

/// Connects the given arguments with an all logic.
/// The content of the iterator is moved out and converted
/// to a temporary `std::vector` which is then passed to when_all.
///
/// ```cpp
/// std::vector<cti::continuable<int>> v{cti::make_ready_continuable(0),
///                                      cti::make_ready_continuable(1)};
///
/// cti::when_all(v.begin(), v.end())
///   .then([](std::vector<int> r01) {
///     // ...
///   });
/// ```
///
/// \param begin The begin iterator to the range which will be moved out
///              and used as the arguments to the all connection
///
/// \param end   The end iterator to the range which will be moved out
///              and used as the arguments to the all connection
///
/// \see         when_all for details.
///
/// \attention   Prefer to invoke when_all with the whole container the
///              iterators were taken from, since this saves us
///              the creation of a temporary storage.
///
/// \since       3.0.0
template <
    typename Iterator,
    std::enable_if_t<detail::range::is_iterator<Iterator>::value>* = nullptr>
auto when_all(Iterator begin, Iterator end) {
  return when_all(detail::range::persist_range(begin, end));
}

/// Connects the given arguments with a sequential logic.
/// All continuables contained inside the given nested pack are
/// invoked one after one. On completion the final handler is called
/// with the aggregated result of all continuables.
///
/// \param args Arbitrary arguments which are connected.
///             Every type is allowed as arguments, continuables may be
///             contained inside tuple like types (`std::tuple`)
///             or in homogeneous containers such as `std::vector`.
///             Non continuable arguments are preserved and passed
///             to the final result as shown below:
/// ```cpp
/// cti::when_seq(
///     cti::make_ready_continuable(0, 1),
///     2, //< See this plain value
///     std::vector<cti::continuable<int>>{cti::make_ready_continuable(3),
///                                        cti::make_ready_continuable(4)},
///     std::make_tuple(std::make_tuple(cti::make_ready_continuable(5))))
///       .then([](int r0, int r1, int r2, std::vector<int> r34,
///                std::tuple<std::tuple<int>> r5) {
///         // ...
///       });
/// ```
///
/// \see        continuable_base::operator>> for details.
///
/// \since      1.1.0
template <typename... Args>
auto when_seq(Args&&... args) {
  return detail::composition::apply_composition(
      detail::composition::composition_strategy_seq_tag{},
      std::forward<Args>(args)...);
}

/// Connects the given arguments with a sequential logic.
/// The content of the iterator is moved out and converted
/// to a temporary `std::vector` which is then passed to when_seq.
///
/// ```cpp
/// std::vector<cti::continuable<int>> v{cti::make_ready_continuable(0),
///                                      cti::make_ready_continuable(1)};
///
/// cti::when_seq(v.begin(), v.end())
///   .then([](std::vector<int> r01) {
///     // ...
///   });
/// ```
///
/// \param begin The begin iterator to the range which will be moved out
///              and used as the arguments to the sequential connection
///
/// \param end   The end iterator to the range which will be moved out
///              and used as the arguments to the sequential connection
///
/// \see         when_seq for details.
///
/// \attention   Prefer to invoke when_seq with the whole container the
///              iterators were taken from, since this saves us
///              the creation of a temporary storage.
///
/// \since       3.0.0
template <
    typename Iterator,
    std::enable_if_t<detail::range::is_iterator<Iterator>::value>* = nullptr>
auto when_seq(Iterator begin, Iterator end) {
  return when_seq(detail::range::persist_range(begin, end));
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
} // namespace cti

#endif // CONTINUABLE_COMPOSITIONS_HPP_INCLUDED
