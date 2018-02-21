/*
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

#include <tuple>

#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/container-category.hpp>
#include <continuable/detail/traits.hpp>

// Devel
#include <continuable/continuable-base.hpp>
#include <vector>

namespace cti {
namespace detail {
/// This namespace provides utilities for performing compound
/// connections between deeply nested continuables and values.
///
/// 0. We create the result pack from the provides values and
///    the async values if those are default constructible,
///    otherwise use a lazy initialization wrapper and unwrap
///    the whole pack when the composition is finished.
///       - value -> value
///       - single async value -> single value
///       - multiple async value -> tuple of async values.
///
/// 1.
namespace remapping {
// Guard object for representing void results
struct void_result_guard {};

/// Contains an continuable together with a location where the
/// result shall be stored.
template <typename Continuable, typename Target>
struct indexed_continuable {
  Continuable continuable;
  Target* target;
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
  static constexpr auto initialize(hints::signature_hint_tag<>) {
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

/// Maps a deeply nested pack of continuables to
struct result_indexer_mapper {
  /// Index a given continuable together with its target location
  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& continuable) {
    auto constexpr const hint = hints::hint_of(traits::identify<T>{});

    using target = decltype(result_extractor_mapper::initialize(hint));

    using type = indexed_continuable<std::decay_t<T>, target>;
    return type{std::forward<T>(continuable), nullptr};
  }
};

/// Relocates the target of a deeply nested pack of indexed_continuable objects
/// to the given target.
struct result_relocator_mapper {
  template <typename Index, typename Result>
  static void traverse(traversal::container_category_tag<false, false>,
                       Index* /*index*/, Result* /*result*/) {
    // Don't do anything when dealing witrh casual objects
  }
  template <typename Continuable, typename Target, typename Result>
  static void traverse(traversal::container_category_tag<false, false>,
                       indexed_continuable<Continuable, Target>* index,
                       Result* result) {

    // Assign the address of the target to the indexed continuable
    index->target = result;
  }

  /// Traverse a homogeneous container
  template <bool IsTupleLike, typename Index, typename Result>
  static void traverse(traversal::container_category_tag<true, IsTupleLike>,
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
  static void traverse_tuple_like(std::integer_sequence<std::size_t, I...>,
                                  Index* index, Result* result) {

    (void)std::initializer_list<int>{(
        (void)traverse(
            traversal::container_category_of_t<decltype(std::get<I>(*index))>{},
            &std::get<I>(*index), &std::get<I>(*result)),
        0)...};
  }

  /// Traverse tuple like container
  template <typename Index, typename Result>
  static void traverse(traversal::container_category_tag<false, true>,
                       Index* index, Result* result) {

    constexpr std::make_index_sequence<std::tuple_size<Index>::value> const i{};
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

/// Returns the result pack of the given deeply nested pack.
/// This invalidates all non-continuable values contained inside the pack.
///
/// This consumes all continuables inside the pack.
template <typename... Args>
constexpr auto create_index_pack(Args&&... args) {
  return cti::map_pack(detail::result_indexer_mapper{},
                       std::forward<Args>(args)...);
}

/// Sets the target pointers of indexed_continuable's inside the index pack
/// to point to their given counterparts inside the given target.
template <typename Index, typename Target>
constexpr void relocate_index_pack(Index* index, Target* target) {
  constexpr traversal::container_category_of_t<Index> const tag;
  detail::result_relocator_mapper::traverse(tag, index, target);
}

/*
template <typename T>
auto remape_container(traversal::container_category_tag<false, true>,
                      T&& container) {
}

template <bool IsTupleLike, typename T>
auto remape_container(traversal::container_category_tag<true, IsTupleLike>,
                      T&& container) {
}

template <
    typename T,
    typename Category = traversal::container_category_of_t<std::decay_t<T>>,
    std::enable_if_t<Category::value>* = nullptr>
auto operator()(T&& container) {
  return remape_container(std::forward<T>(container));
}
 */
} // namespace remapping

struct c {};

template <typename C, typename... Args>
struct loc {};

struct runtime_insertion {
  std::size_t begin, end;
};

template <typename... Args>
struct future_result {
  std::tuple<Args...> result_;
};
} // namespace detail
} // namespace cti

using namespace cti::detail::remapping;

int main(int, char**) {

  using namespace cti::detail;

  std::vector<int> vc{1, 2, 3};

  // std::tuple<c, c, c> t;
  // std::tuple<loc<c, ct<0>>, c, c> loc;

  auto p =
      std::make_tuple(0, 4, cti::make_ready_continuable(0),
                      std::make_tuple(1, 2), cti::make_ready_continuable(0));

  auto r = create_result_pack(std::move(p));
  auto i = create_index_pack(std::move(p));

  relocate_index_pack(&i, &r);

  return 0;
}
