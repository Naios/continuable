
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

#ifndef CONTINUABLE_CONNECTIONS_HPP_INCLUDED
#define CONTINUABLE_CONNECTIONS_HPP_INCLUDED

#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>
#include <continuable/detail/connection/connection-all.hpp>
#include <continuable/detail/connection/connection-any.hpp>
#include <continuable/detail/connection/connection-seq.hpp>
#include <continuable/detail/connection/connection.hpp>
#include <continuable/detail/traversal/range.hpp>

namespace cti {
/// \defgroup Connections Connections
/// provides functions to connect \link continuable_base
/// continuable_bases\endlink through various strategies.
/// \{

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
///     cti::populate(cti::make_ready_continuable(3),  // Creates a runtime
///                   cti::make_ready_continuable(4)), // sized container.
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
  return detail::connection::apply_connection(
      detail::connection::connection_strategy_all_tag{},
      std::forward<Args>(args)...);
}

/// Connects the given arguments with an all logic.
/// The content of the iterator is moved out and converted
/// to a temporary `std::vector` which is then passed to when_all.
///
/// ```cpp
/// // cti::populate just creates a std::vector from the two continuables.
/// auto v = cti::populate(cti::make_ready_continuable(0),
///                        cti::make_ready_continuable(1));
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
///     cti::populate(cti::make_ready_continuable(3),  // Creates a runtime
///                   cti::make_ready_continuable(4)), // sized container.
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
  return detail::connection::apply_connection(
      detail::connection::connection_strategy_seq_tag{},
      std::forward<Args>(args)...);
}

/// Connects the given arguments with a sequential logic.
/// The content of the iterator is moved out and converted
/// to a temporary `std::vector` which is then passed to when_seq.
///
/// ```cpp
/// // cti::populate just creates a std::vector from the two continuables.
/// auto v = cti::populate(cti::make_ready_continuable(0),
///                        cti::make_ready_continuable(1));
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

/// Connects the given arguments with an any logic.
/// All continuables contained inside the given nested pack are
/// invoked at once. On completion of one continuable the final handler
/// is called with the result of the resolved continuable.
///
/// \param args Arbitrary arguments which are connected.
///             Every type is allowed as arguments, continuables may be
///             contained inside tuple like types (`std::tuple`)
///             or in homogeneous containers such as `std::vector`.
///             Non continuable arguments are preserved and passed
///             to the final result as shown below:
/// ```cpp
/// cti::when_any(
///     cti::make_ready_continuable(0, 1),
///     2, //< See this plain value
///     cti::populate(cti::make_ready_continuable(3),  // Creates a runtime
///                   cti::make_ready_continuable(4)), // sized container.
///     std::make_tuple(std::make_tuple(cti::make_ready_continuable(5))))
///       .then([](int r0) {
///         // ...
///       });
/// ```
///
/// \see        continuable_base::operator|| for details.
///
/// \since      1.1.0
template <typename... Args>
auto when_any(Args&&... args) {
  return detail::connection::apply_connection(
      detail::connection::connection_strategy_any_tag{},
      std::forward<Args>(args)...);
}

/// Connects the given arguments with an any logic.
/// The content of the iterator is moved out and converted
/// to a temporary `std::vector` which is then passed to when_all.
///
/// ```cpp
/// // cti::populate just creates a std::vector from the two continuables.
/// auto v = cti::populate(cti::make_ready_continuable(0),
///                        cti::make_ready_continuable(1));
///
/// cti::when_any(v.begin(), v.end())
///   .then([](int r01) {
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
/// \see         when_any for details.
///
/// \attention   Prefer to invoke when_any with the whole container the
///              iterators were taken from, since this saves us
///              the creation of a temporary storage.
///
/// \since       3.0.0
template <
    typename Iterator,
    std::enable_if_t<detail::range::is_iterator<Iterator>::value>* = nullptr>
auto when_any(Iterator begin, Iterator end) {
  return when_any(detail::range::persist_range(begin, end));
}

/// Populates a homogeneous container from the given arguments.
/// All arguments need to be convertible to the first one,
/// by default `std::vector` is used as container type.
///
/// This method mainly helps to create a homogeneous container from
/// a runtime known count of continuables which type isn't exactly known.
/// All continuables which are passed to this function should be originating
/// from the same source or a method called with the same types of arguments:
/// ```cpp
/// auto container = cti::populate(cti::make_ready_continuable(0),
///                                cti::make_ready_continuable(1)),
///
/// for (int i = 2; i < 5; ++i) {
///   // You may add more continuables to the container afterwards
///   container.emplace_back(cti::make_ready_continuable(i));
/// }
///
/// cti::when_any(std::move(container))
///   .then([](int) {
///     // ...
///   });
/// ```
/// Additionally it is possible to change the targeted container as below:
/// ```cpp
/// auto container = cti::populate<std::list>(cti::make_ready_continuable(0),
///                                           cti::make_ready_continuable(1)),
/// ```
///
/// \tparam C The container type which is used to store the arguments into.
///
/// \since    3.0.0
template <template <typename, typename> class C = std::vector, typename First,
          typename... Args>
C<std::decay_t<First>, std::allocator<std::decay_t<First>>>
populate(First&& first, Args&&... args) {
  C<std::decay_t<First>, std::allocator<std::decay_t<First>>> container;
  container.reserve(1 + sizeof...(Args));
  container.emplace_back(std::forward<First>(first));
  (void)std::initializer_list<int>{
      0, ((void)container.emplace_back(std::forward<Args>(args)), 0)...};
  return container; // RVO
}
/// \}
} // namespace cti

#endif // CONTINUABLE_CONNECTIONS_HPP_INCLUDED
