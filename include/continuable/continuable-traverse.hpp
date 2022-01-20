
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

#ifndef CONTINUABLE_TRAVERSE_HPP_INCLUDED
#define CONTINUABLE_TRAVERSE_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/detail/traversal/traverse.hpp>

namespace cti {
/// \defgroup Traversal Traversal
/// provides functions to traverse and remap nested packs.
/// \{

/// Maps the pack with the given mapper.
///
/// This function tries to visit all plain elements which may be wrapped in:
/// - homogeneous containers (`std::vector`, `std::list`)
/// - heterogenous containers `(std::tuple`, `std::pair`, `std::array`)
/// and re-assembles the pack with the result of the mapper.
/// Mapping from one type to a different one is supported.
///
/// Elements that aren't accepted by the mapper are routed through
/// and preserved through the hierarchy.
///
///    ```cpp
///    // Maps all integers to floats
///    map_pack([](int value) {
///      return float(value);
///    },
///    1, std::make_tuple(2, std::vector<int>{3, 4}), 5);
///    ```
///
/// \throws       std::exception like objects which are thrown by an
///               invocation to the mapper.
///
/// \param mapper A callable object, which accept an arbitrary type
///               and maps it to another type or the same one.
///
/// \param pack   An arbitrary variadic pack which may contain any type.
///
/// \returns      The mapped element or in case the pack contains
///               multiple elements, the pack is wrapped into
///               a `std::tuple`.
///
/// \since        3.0.0
///
template <typename Mapper, typename... T>
/*keep this inline*/ inline decltype(auto) map_pack(Mapper&& mapper,
                                                    T&&... pack) {
  return detail::traversal::transform(detail::traversal::strategy_remap_tag{},
                                      std::forward<Mapper>(mapper),
                                      std::forward<T>(pack)...);
}

/// Indicate that the result shall be spread across the parent container
/// if possible. This can be used to create a mapper function used
/// in map_pack that maps one element to an arbitrary count (1:n).
///
/// \since 3.0.0
template <typename... T>
constexpr detail::traversal::spreading::spread_box<std::decay_t<T>...>
spread_this(T&&... args) noexcept(
    noexcept(std::make_tuple(std::forward<T>(args)...))) {
  using type = detail::traversal::spreading::spread_box<std::decay_t<T>...>;
  return type(std::make_tuple(std::forward<T>(args)...));
}

/// Traverses the pack with the given visitor.
///
/// This function works in the same way as `map_pack`,
/// however, the result of the mapper isn't preserved.
///
/// See `map_pack` for a detailed description.
///
/// \since 3.0.0
template <typename Mapper, typename... T>
void traverse_pack(Mapper&& mapper, T&&... pack) {
  detail::traversal::transform(detail::traversal::strategy_traverse_tag{},
                               std::forward<Mapper>(mapper),
                               std::forward<T>(pack)...);
}
/// \}
} // namespace cti

#endif // CONTINUABLE_TRAVERSE_HPP_INCLUDED
