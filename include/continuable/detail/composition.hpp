
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

#ifndef CONTINUABLE_DETAIL_COMPOSITION_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
namespace detail {
/// The namespace `composition` offers methods to chain continuations together
/// with `all`, `any` or `seq` logic.
namespace composition {
struct composition_strategy_all_tag {};
struct composition_strategy_any_tag {};
struct composition_strategy_seq_tag {};

template <typename T>
struct is_composition_strategy // ...
    : std::false_type {};
template <>
struct is_composition_strategy<composition_strategy_all_tag> // ...
    : std::true_type {};
template <>
struct is_composition_strategy<composition_strategy_any_tag> // ...
    : std::true_type {};

/// Adds the given continuation tuple to the left composition
template <typename... LeftArgs, typename... RightArgs>
auto chain_composition(std::tuple<LeftArgs...> leftPack,
                       std::tuple<RightArgs...> rightPack) {

  return traits::merge(std::move(leftPack), std::move(rightPack));
}

/// Normalizes a continuation to a tuple holding an arbitrary count of
/// continuations matching the given strategy.
///
/// Basically we can encounter 3 cases:
/// - The continuable isn't in any strategy:
///   -> make a tuple containing the continuable as only element
template <
    typename Strategy, typename Data, typename Annotation,
    std::enable_if_t<!is_composition_strategy<Annotation>::value>* = nullptr>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Annotation>&& continuation) {

  // If the continuation isn't a strategy initialize the strategy
  return std::make_tuple(std::move(continuation));
}
/// - The continuable is in a different strategy then the current one:
///   -> materialize it
template <
    typename Strategy, typename Data, typename Annotation,
    std::enable_if_t<is_composition_strategy<Annotation>::value>* = nullptr>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Annotation>&& continuation) {

  // If the right continuation is a different strategy materialize it
  // in order to keep the precedence in cases where: `c1 && (c2 || c3)`.
  return std::make_tuple(base::attorney::materialize(std::move(continuation)));
}
/// - The continuable is inside the current strategy state:
///   -> return the data of the tuple
template <typename Strategy, typename Data>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Strategy>&& continuation) {

  // If we are in the given strategy we can just use the data of the continuable
  return base::attorney::consume_data(std::move(continuation));
}

/// Entry function for connecting two continuables with a given strategy.
template <typename Strategy, typename LData, typename LAnnotation,
          typename RData, typename RAnnotation>
auto connect(Strategy strategy, continuable_base<LData, LAnnotation>&& left,
             continuable_base<RData, RAnnotation>&& right) {

  auto ownership_ =
      base::attorney::ownership_of(left) | base::attorney::ownership_of(right);

  left.freeze();
  right.freeze();

  // Make the new data which consists of a tuple containing
  // all connected continuables.
  auto data = chain_composition(normalize(strategy, std::move(left)),
                                normalize(strategy, std::move(right)));

  // Return a new continuable containing the tuple and holding
  // the current strategy as annotation.
  return base::attorney::create(std::move(data), strategy, ownership_);
}

/// All strategies should specialize this class in order to provide:
/// - A finalize static method that creates the callable object which
///   is invoked with the callback to call when the composition is finished.
/// - A static method hint that returns the new signature hint.
template <typename Strategy>
struct composition_finalizer;

struct aggregate_ownership {
  util::ownership& ownership_;

  template <typename Continuable,
            std::enable_if_t<base::is_continuable<
                std::decay_t<Continuable>>::value>* = nullptr>
  auto operator()(Continuable const& continuable) noexcept {
    util::ownership other = base::attorney::ownership_of(continuable);
    /*if (!other.is_default())*/ { ownership_ |= other; }
  }
};

// Merges the ownership of all continuables involved into the strategy
/*if (ownership.is_acquired() || !ownership.is_frozen()) {
  traverse_pack(aggregate_ownership{ownership}, composition);
}*/

/// Finalizes the any logic of a given composition
template <typename Data, typename Strategy>
auto finalize_composition(continuable_base<Data, Strategy>&& continuation) {
  using finalizer = composition_finalizer<Strategy>;

  util::ownership ownership = base::attorney::ownership_of(continuation);
  auto composition = base::attorney::consume_data(std::move(continuation));

  // Retrieve the new signature hint
  constexpr auto const signature =
      finalizer::template hint<decltype(composition)>();

  // Return a new continuable which
  return base::attorney::create(finalizer::finalize(std::move(composition)),
                                signature, std::move(ownership));
}
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_HPP_INCLUDED
