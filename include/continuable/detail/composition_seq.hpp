
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

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-traverse-async.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/composition_all.hpp>
#include <continuable/detail/composition_remapping.hpp>
#include <continuable/detail/traits.hpp>

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

/// Contains an continuable together with a location where the
/// result shall be stored.
template <typename Continuable, typename Target>
struct indexed_continuable {
  Continuable continuable;
  Target* target;
};

template <typename T>
struct is_indexed_continuable : std::false_type {};
template <typename Continuable, typename Target>
struct is_indexed_continuable<indexed_continuable<Continuable, Target>>
    : std::true_type {};

/// Maps a deeply nested pack of continuables to an indexed continuable
struct result_indexer_mapper {
  /// Index a given continuable together with its target location
  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& continuable) {
    auto constexpr const hint = hints::hint_of(traits::identify<T>{});

    using target =
        decltype(remapping::detail::result_extractor_mapper::initialize(hint));

    using type = indexed_continuable<std::decay_t<T>, target>;
    return type{std::forward<T>(continuable), nullptr};
  }
};

/// Returns the result pack of the given deeply nested pack.
/// This invalidates all non-continuable values contained inside the pack.
///
/// This consumes all continuables inside the pack.
template <typename... Args>
constexpr auto create_index_pack(Args&&... args) {
  return cti::map_pack(result_indexer_mapper{}, std::forward<Args>(args)...);
}

struct index_relocator {
  template <typename Index, typename Target,
            std::enable_if_t<
                is_indexed_continuable<std::decay_t<Index>>::value>* = nullptr>
  auto operator()(Index* index, Target* target) const noexcept {
    // Assign the address of the target to the indexed continuable
    index->target = target;
  }
};

constexpr remapping::void_result_guard wrap() {
  return {};
}
template <typename First>
constexpr decltype(auto) wrap(First&& first) {
  return std::forward<First>(first);
}
template <typename First, typename Second, typename... Rest>
constexpr decltype(auto) wrap(First&& first, Second&& second, Rest&&... rest) {
  return std::make_tuple(std::forward<First>(first),
                         std::forward<Second>(second),
                         std::forward<Rest>(rest)...);
}

template <typename Callback, typename Index, typename Result>
struct sequential_dispatch_data {
  Callback callback;
  Index index;
  Result result;
};

template <typename Data>
class sequential_dispatch_visitor
    : public std::enable_shared_from_this<sequential_dispatch_visitor<Data>> {

  Data data_;

public:
  explicit sequential_dispatch_visitor(Data&& data) : data_(std::move(data)) {
    // Assign the address of each result target to the corresponding
    // indexed continuable.
    remapping::relocate_index_pack(index_relocator{}, &data_.index,
                                   &data_.result);
  }

  virtual ~sequential_dispatch_visitor() = default;

  /// Returns the pack that should be traversed
  auto& head() {
    return data_.index;
  }

  template <typename Index, std::enable_if_t<is_indexed_continuable<
                                std::decay_t<Index>>::value>* = nullptr>
  bool operator()(async_traverse_visit_tag, Index&& /*index*/) {
    return false;
  }

  template <typename Index, typename N>
  void operator()(async_traverse_detach_tag, Index&& index, N&& next) {
    assert(index.target && "The target should be non null here!"
                           "Probably this is caused through a bug in "
                           "result_relocator_mapper!");

    std::move(index.continuable)
        .then([ target = index.target,
                next = std::forward<N>(next) ](auto&&... args) mutable {

          // Assign the result to the target
          *target = wrap(std::forward<decltype(args)>(args)...);

          // Continue the asynchronous sequential traversal
          next();
        })
        .done();
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& /*pack*/) {
    // Remove void result guard tags
    auto cleaned =
        map_pack(remapping::unpack_result_guards{}, std::move(data_.result));

    // Call the final callback with the cleaned result
    traits::unpack(std::move(cleaned), std::move(data_.callback));
  }
};
} // namespace seq

/// Finalizes the seq logic of a given composition
template <>
struct composition_finalizer<composition_strategy_seq_tag> {
  template <typename Composition>
  static constexpr auto hint() {
    // The result is the same as in the all composition
    using all_finalizer = composition_finalizer<composition_strategy_all_tag>;
    return all_finalizer::hint<Composition>();
  }

  /// Finalizes the all logic of a given composition
  template <typename Composition>
  static auto finalize(Composition&& composition) {
    return [composition = std::forward<Composition>(composition)](
        auto&& callback) mutable {

      auto index = seq::create_index_pack(composition);
      auto result =
          remapping::create_result_pack(std::forward<Composition>(composition));

      // The data from which the visitor is constructed in-place
      using data_t =
          seq::sequential_dispatch_data<std::decay_t<decltype(callback)>,
                                        std::decay_t<decltype(index)>,
                                        std::decay_t<decltype(result)>>;

      // The visitor type
      using visitor_t = seq::sequential_dispatch_visitor<data_t>;

      traverse_pack_async(async_traverse_in_place_tag<visitor_t>{},
                          data_t{std::forward<decltype(callback)>(callback),
                                 std::move(index), std::move(result)});
    };
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_SEQ_HPP_INCLUDED
