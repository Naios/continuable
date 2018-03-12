
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

#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/flat-variant.hpp>
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
namespace aggregated {
/// Guards a type to be default constructible,
/// and wraps it into an optional type if it isn't default constructible.
template <typename T>
using lazy_value_t = std::conditional_t<std::is_default_constructible<T>::value,
                                        T, container::flat_variant<T>>;

template <typename T>
decltype(auto) unpack_lazy(T&& value) {
  return std::forward<T>(value);
}
template <typename T>
T&& unpack_lazy(container::flat_variant<T>&& value) {
  assert(value.template is<T>() &&
         "The composition was finalized before all values were present!");

  return std::move(value.template cast<T>());
}

template <typename Continuable>
class continuable_box;
template <typename Data>
class continuable_box<continuable_base<Data, hints::signature_hint_tag<>>> {

  continuable_base<Data, hints::signature_hint_tag<>> continuable_;

public:
  explicit continuable_box(
      continuable_base<Data, hints::signature_hint_tag<>>&& continuable)
      : continuable_(std::move(continuable)) {
  }

  continuable_base<Data, hints::signature_hint_tag<>>&& fetch() {
    return std::move(continuable_);
  }

  void assign() {
  }

  auto unbox() && {
    return spread_this();
  }
};
template <typename Data, typename First>
class continuable_box<
    continuable_base<Data, hints::signature_hint_tag<First>>> {

  continuable_base<Data, hints::signature_hint_tag<First>> continuable_;
  lazy_value_t<First> first_;

public:
  explicit continuable_box(
      continuable_base<Data, hints::signature_hint_tag<First>>&& continuable)
      : continuable_(std::move(continuable)) {
  }

  continuable_base<Data, hints::signature_hint_tag<First>>&& fetch() {
    return std::move(continuable_);
  }

  void assign(First first) {
    first_ = std::move(first);
  }

  auto unbox() && {
    return unpack_lazy(std::move(first_));
  }
};
template <typename Data, typename First, typename Second, typename... Rest>
class continuable_box<
    continuable_base<Data, hints::signature_hint_tag<First, Second, Rest...>>> {

  continuable_base<Data, hints::signature_hint_tag<First, Second, Rest...>>
      continuable_;
  lazy_value_t<std::tuple<First, Second, Rest...>> args_;

public:
  explicit continuable_box(
      continuable_base<Data,
                       hints::signature_hint_tag<First, Second, Rest...>>&&
          continuable)
      : continuable_(std::move(continuable)) {
  }

  continuable_base<Data, hints::signature_hint_tag<First, Second, Rest...>>&&
  fetch() {
    return std::move(continuable_);
  }

  void assign(First first, Second second, Rest... rest) {
    args_ = std::make_tuple(std::move(first), std::move(second),
                            std::move(rest)...);
  }

  auto unbox() && {
    return traits::unpack(unpack_lazy(std::move(args_)), [](auto&&... args) {
      return spread_this(std::forward<decltype(args)>(args)...);
    });
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
constexpr auto finalize_impl(traits::identity<void>, Callback&& callback,
                             Data&&) {
  return std::forward<Callback>(callback)();
}
template <typename... Args, typename Callback, typename Data>
constexpr auto finalize_impl(traits::identity<std::tuple<Args...>>,
                             Callback&& callback, Data&& data) {
  // Call the final callback with the cleaned result
  return traits::unpack(unbox_continuables(std::forward<Data>(data)),
                        std::forward<Callback>(callback));
}

struct hint_mapper {
  template <typename... T>
  constexpr auto operator()(T...) -> hints::signature_hint_tag<T...> {
    return {};
  }
};
} // namespace detail

template <typename Callback, typename Data>
constexpr auto finalize_data(Callback&& callback, Data&& data) {
  using result_t = decltype(unbox_continuables(std::forward<Data>(data)));
  // Guard the final result against void
  return detail::finalize_impl(traits::identity<std::decay_t<result_t>>{},
                               std::forward<Callback>(callback),
                               std::forward<Data>(data));
}

template <typename Data>
constexpr auto hint_of_data() {
  return decltype(finalize_data(detail::hint_mapper{}, std::declval<Data>())){};
}
} // namespace aggregated
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_REMAPPING_HPP_INCLUDED
