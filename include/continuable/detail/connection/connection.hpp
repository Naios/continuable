
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

#ifndef CONTINUABLE_DETAIL_CONNECTION_HPP_INCLUDED
#define CONTINUABLE_DETAIL_CONNECTION_HPP_INCLUDED

#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
/// The namespace `connection` offers methods to chain continuations together
/// with `all`, `any` or `seq` logic.
namespace connection {
template <typename T>
struct is_connection_strategy // ...
    : std::false_type {};

/// Adds the given continuation tuple to the left connection
template <typename... LeftArgs, typename... RightArgs>
auto chain_connection(std::tuple<LeftArgs...> leftPack,
                      std::tuple<RightArgs...> rightPack) {

  return std::tuple_cat(std::move(leftPack), std::move(rightPack));
}

/// Normalizes a continuation to a tuple holding an arbitrary count of
/// continuations matching the given strategy.
///
/// Basically we can encounter 3 cases:
/// - The continuable isn't in any strategy:
///   -> make a tuple containing the continuable as only element
template <
    typename Strategy, typename Data, typename Annotation,
    std::enable_if_t<!is_connection_strategy<Annotation>::value>* = nullptr>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Annotation>&& continuation) {

  // If the continuation isn't a strategy initialize the strategy
  return std::make_tuple(std::move(continuation));
}
/// - The continuable is in a different strategy then the current one:
///   -> materialize it
template <
    typename Strategy, typename Data, typename Annotation,
    std::enable_if_t<is_connection_strategy<Annotation>::value>* = nullptr>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Annotation>&& continuation) {

  // If the right continuation is a different strategy materialize it
  // in order to keep the precedence in cases where: `c1 && (c2 || c3)`.
  return std::make_tuple(std::move(continuation).finish());
}
/// - The continuable is inside the current strategy state:
///   -> return the data of the tuple
template <typename Strategy, typename Data>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Strategy>&& continuation) {

  // If we are in the given strategy we can just use the data of the continuable
  return base::attorney::consume(std::move(continuation));
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
  auto data = chain_connection(normalize(strategy, std::move(left)),
                               normalize(strategy, std::move(right)));

  // Return a new continuable containing the tuple and holding
  // the current strategy as annotation.
  return base::attorney::create_from_raw(std::move(data), strategy, ownership_);
}

/// All strategies should specialize this class in order to provide:
/// - A finalize static method that creates the callable object which
///   is invoked with the callback to call when the connection is finished.
/// - A static method hint that returns the new signature hint.
template <typename Strategy>
struct connection_finalizer;

template <typename Strategy>
struct connection_annotation_trait {
  /// Finalizes the connection logic of a given connection
  template <typename Continuable>
  static auto finish(Continuable&& continuable) {
    using finalizer = connection_finalizer<Strategy>;

    util::ownership ownership = base::attorney::ownership_of(continuable);
    auto connection =
        base::attorney::consume(std::forward<Continuable>(continuable));

    // Return a new continuable which
    return finalizer::finalize(std::move(connection), std::move(ownership));
  }

  template <typename Continuable>
  static bool is_ready(Continuable const& /*continuable*/) noexcept {
    return false;
  }
};

class prepare_continuables {
  util::ownership& ownership_;

public:
  explicit constexpr prepare_continuables(util::ownership& ownership)
      : ownership_(ownership) {
  }

  template <typename Continuable,
            std::enable_if_t<base::is_continuable<
                std::decay_t<Continuable>>::value>* = nullptr>
  auto operator()(Continuable&& continuable) noexcept {
    util::ownership current = base::attorney::ownership_of(continuable);
    assert(current.is_acquired() &&
           "Only valid continuables should be passed!");

    // Propagate a frozen state to the new continuable
    if (!ownership_.is_frozen() && current.is_frozen()) {
      ownership_.freeze();
    }

    // Freeze the continuable since it is stored for later usage
    continuable.freeze();

    // Materialize every continuable
    // TODO Actually we would just need to consume the data here
    return std::forward<Continuable>(continuable).finish();
  }
};

template <typename Strategy, typename... Args>
auto apply_connection(Strategy, Args&&... args) {
  using finalizer = connection_finalizer<Strategy>;

  // Freeze every continuable inside the given arguments,
  // and freeze the ownership if one of the continuables
  // is frozen already.
  // Additionally test whether every continuable is acquired.
  // Also materialize every continuable.
  util::ownership ownership;
  auto connection = map_pack(prepare_continuables{ownership},
                             std::make_tuple(std::forward<Args>(args)...));

  return finalizer::finalize(std::move(connection), std::move(ownership));
}
} // namespace connection
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_CONNECTION_HPP_INCLUDED
