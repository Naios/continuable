
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

#ifndef CONTINUABLE_DETAIL_COMPOSITION_REMAPPING_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_REMAPPING_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/container-category.hpp>
#include <continuable/detail/traits.hpp>

namespace cti {
namespace detail {
namespace composition {
/// This namespace provides utilities for performing compound
/// connections between deeply nested continuables and values.
///
/// We create the result pack from the provides values and
/// the async values if those are default constructible,
/// otherwise use a lazy initialization wrapper and unwrap
/// the whole pack when the composition is finished.
///   - value -> value
///   - single async value -> single value
///   - multiple async value -> tuple of async values.
namespace remapping {
// Guard object for representing void results
struct void_result_guard {};

// Callable object that maps void_result_guard zo zero arguments
struct clean_void_results {
  auto operator()(void_result_guard) const noexcept {
    return spread_this();
  }
};

namespace detail {
struct result_extractor_mapper {
  /// Create slots for a void result which is removed later.
  /// This is required due to the fact that each continuable has exactly
  /// one matching valuen inside the result tuple.
  static constexpr auto initialize(hints::signature_hint_tag<>) noexcept {
    return void_result_guard{};
  }
  /// Initialize a single value
  template <typename First>
  static constexpr auto initialize(hints::signature_hint_tag<First>) {
    return First{};
  }
  /// Initialize a multiple values as tuple
  template <typename First, typename Second, typename... Args>
  static constexpr auto
  initialize(hints::signature_hint_tag<First, Second, Args...>) {
    // TODO Fix non default constructible values
    return std::make_tuple(First{}, Second{}, Args{}...);
  }

  /// Remap a continuable to its corresponding result values
  /// A void result is mapped to a guard type, single values to the value
  /// itself and multiple ones to a tuple of values.
  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& /*continuable*/) {
    auto constexpr const hint = hints::hint_of(traits::identify<T>{});
    return initialize(hint);
  }
};

/// Relocates the target of a deeply nested pack of indexed_continuable objects
/// to the given target.
template <typename Evaluator>
struct result_relocator_mapper {
  Evaluator evaluator;

  template <typename Index, typename Result>
  void traverse_one(std::false_type, Index*, Result*) {
    // Don't do anything when dealing with casual objects
  }
  template <typename Index, typename Result>
  void traverse_one(std::true_type, Index* index, Result* result) {

    // Call the evaluator with the address of the indexed object and its target
    evaluator(index, result);
  }
  template <typename Index, typename Result>
  void traverse(traversal::container_category_tag<false, false>, Index* index,
                Result* result) {

    traverse_one(traits::is_invocable<Evaluator, Index, Result>{}, index,
                 result);
  }

  /// Traverse a homogeneous container
  template <bool IsTupleLike, typename Index, typename Result>
  void traverse(traversal::container_category_tag<true, IsTupleLike>,
                Index* index, Result* result) {
    auto index_itr = index->begin();
    auto const index_end = index->end();

    auto result_itr = result->begin();
    auto const result_end = result->end();

    using element_t = std::decay_t<decltype(*index->begin())>;
    traversal::container_category_of_t<element_t> constexpr const tag;

    for (; index_itr != index_end; ++index_itr, ++result_itr) {
      assert(result_itr != result_end);
      traverse(tag, &*index_itr, &*result_itr);
    }
  }

  template <std::size_t... I, typename Index, typename Result>
  void traverse_tuple_like(std::integer_sequence<std::size_t, I...>,
                           Index* index, Result* result) {

    (void)std::initializer_list<int>{(
        (void)traverse(
            traversal::container_category_of_t<decltype(std::get<I>(*index))>{},
            &std::get<I>(*index), &std::get<I>(*result)),
        0)...};
  }

  /// Traverse tuple like container
  template <typename Index, typename Result>
  void traverse(traversal::container_category_tag<false, true>, Index* index,
                Result* result) {

    std::make_index_sequence<std::tuple_size<Index>::value> constexpr const i{};

    traverse_tuple_like(i, index, result);
  }
};
} // namespace detail

/// Returns the result pack of the given deeply nested pack.
/// This invalidates all non-continuable values contained inside the pack.
///
/// This consumes all non continuables inside the pack.
template <typename... Args>
constexpr auto create_result_pack(Args&&... args) {
  return cti::map_pack(detail::result_extractor_mapper{},
                       std::forward<Args>(args)...);
}

/// Sets the target pointers of indexed_continuable's inside the index pack
/// to point to their given counterparts inside the given target.
template <typename Relocator, typename Index, typename Target>
constexpr void relocate_index_pack(Relocator&& relocator, Index* index,
                                   Target* target) {

  constexpr traversal::container_category_of_t<Index> const tag;

  detail::result_relocator_mapper<std::decay_t<Relocator>> mapper{
      std::forward<Relocator>(relocator)};

  mapper.traverse(tag, index, target);
}
} // namespace remapping
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_REMAPPING_HPP_INCLUDED
