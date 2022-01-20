
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

#ifndef CONTINUABLE_DETAIL_CONNECTION_REMAPPING_HPP_INCLUDED
#define CONTINUABLE_DETAIL_CONNECTION_REMAPPING_HPP_INCLUDED

#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/continuable-result.hpp>
#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace connection {
/// This namespace provides utilities for performing compound
/// connections between deeply nested continuables and values.
///
/// We create the result pack from the provides values and
/// the async values if those are default constructible,
/// otherwise use a lazy initialization wrapper and unwrap
/// the whole pack when the connection is finished.
///   - value -> value
///   - single async value -> single value
///   - multiple async value -> tuple of async values.
namespace aggregated {

/// Guards a type to be default constructible,
/// and wraps it into an optional type if it isn't default constructible.
template <typename T>
using lazy_value_t = std::conditional_t<std::is_default_constructible<T>::value,
                                        T, result<T>>;

template <typename T>
decltype(auto) unpack_lazy(std::true_type /*is_default_constructible*/,
                           T&& value) {
  return std::forward<T>(value);
}
template <typename T>
T&& unpack_lazy(std::false_type /*is_default_constructible*/,
                result<T>&& value) {
  assert(value.is_value() &&
         "The connection was finalized before all values were present!");

  return std::move(value).get_value();
}

template <typename Continuable>
class continuable_box;
template <typename Data>
class continuable_box<continuable_base<Data, identity<>>> {

  continuable_base<Data, identity<>> continuable_;

public:
  explicit continuable_box(continuable_base<Data, identity<>>&& continuable)
    : continuable_(std::move(continuable)) {}

  auto const& peek() const {
    return continuable_;
  }

  auto&& fetch() {
    return std::move(continuable_);
  }

  void assign() {}

  auto unbox() && {
    return spread_this();
  }
};

template <typename Data, typename First>
class continuable_box<continuable_base<Data, identity<First>>> {

  continuable_base<Data, identity<First>> continuable_;
  lazy_value_t<First> first_;

public:
  explicit continuable_box(
      continuable_base<Data, identity<First>>&& continuable)
    : continuable_(std::move(continuable)) {}

  auto const& peek() const {
    return continuable_;
  }

  auto&& fetch() {
    return std::move(continuable_);
  }

  void assign(First first) {
    first_ = std::move(first);
  }

  auto unbox() && {
    return unpack_lazy(std::is_default_constructible<First>{},
                       std::move(first_));
  }
};
template <typename Data, typename First, typename Second, typename... Rest>
class continuable_box<
    continuable_base<Data, identity<First, Second, Rest...>>> {

  continuable_base<Data, identity<First, Second, Rest...>> continuable_;
  lazy_value_t<std::tuple<First, Second, Rest...>> args_;

public:
  explicit continuable_box(
      continuable_base<Data, identity<First, Second, Rest...>>&& continuable)
    : continuable_(std::move(continuable)) {}

  auto const& peek() const {
    return continuable_;
  }

  auto&& fetch() {
    return std::move(continuable_);
  }

  void assign(First first, Second second, Rest... rest) {
    args_ = std::make_tuple(std::move(first), std::move(second),
                            std::move(rest)...);
  }

  auto unbox() && {
    return traits::unpack(
        [](auto&&... args) {
          return spread_this(std::forward<decltype(args)>(args)...);
        },
        unpack_lazy(
            std::is_default_constructible<std::tuple<First, Second, Rest...>>{},
            std::move(args_)));
  }
};

template <typename T>
struct is_continuable_box : std::false_type {};
template <typename Continuable>
struct is_continuable_box<continuable_box<Continuable>> : std::true_type {};

namespace detail {
/// Maps a deeply nested pack of continuables to a continuable_box
struct continuable_box_packer {
  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& continuable) {
    return continuable_box<std::decay_t<T>>{std::forward<T>(continuable)};
  }
};
/// Maps a deeply nested pack of continuable_boxes to its result
struct continuable_box_unpacker {
  template <
      typename T,
      std::enable_if_t<is_continuable_box<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& box) {
    return std::forward<T>(box).unbox();
  }
};
} // namespace detail

/// Returns the boxed pack of the given deeply nested pack.
/// This transforms all continuables into a continuable_box which is
/// capable of caching the result from the corresponding continuable.
template <typename... Args>
constexpr auto box_continuables(Args&&... args) {
  return cti::map_pack(detail::continuable_box_packer{},
                       std::forward<Args>(args)...);
}

/// Returns the unboxed pack of the given deeply nested boxed pack.
/// This transforms all continuable_boxes into its result.
template <typename... Args>
constexpr auto unbox_continuables(Args&&... args) {
  return cti::map_pack(detail::continuable_box_unpacker{},
                       std::forward<Args>(args)...);
}

namespace detail {
template <typename Callback, typename Data>
constexpr auto finalize_impl(identity<void>, Callback&& callback, Data&&) {
  return std::forward<Callback>(callback)();
}
template <typename... Args, typename Callback, typename Data>
constexpr auto finalize_impl(identity<std::tuple<Args...>>, Callback&& callback,
                             Data&& data) {
  // Call the final callback with the cleaned result
  return traits::unpack(std::forward<Callback>(callback),
                        unbox_continuables(std::forward<Data>(data)));
}

struct hint_mapper {
  template <typename... T>
  constexpr auto operator()(T...) -> identity<T...> {
    return {};
  }
};
} // namespace detail

template <typename Callback, typename Data>
constexpr auto finalize_data(Callback&& callback, Data&& data) {
  using result_t = decltype(unbox_continuables(std::forward<Data>(data)));
  // Guard the final result against void
  return detail::finalize_impl(identity<std::decay_t<result_t>>{},
                               std::forward<Callback>(callback),
                               std::forward<Data>(data));
}

template <typename Data>
constexpr auto hint_of_data() {
  return decltype(finalize_data(detail::hint_mapper{}, std::declval<Data>())){};
}
} // namespace aggregated
} // namespace connection
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_CONNECTION_REMAPPING_HPP_INCLUDED
