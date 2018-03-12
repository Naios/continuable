
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

#ifndef CONTINUABLE_DETAIL_COMPOSITION_ANY_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_ANY_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/continuable-promise-base.hpp>
#include <continuable/continuable-traverse.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/container-category.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace composition {
namespace any {
/// Invokes the callback with the first arriving result
template <typename T>
class any_result_submitter
    : public std::enable_shared_from_this<any_result_submitter<T>>,
      public util::non_movable {

  T callback_;
  std::once_flag flag_;

  struct any_callback {
    std::shared_ptr<any_result_submitter> me_;

    template <typename... PartialArgs>
    void operator()(PartialArgs&&... args) && {
      me_->invoke(std::forward<decltype(args)>(args)...);
    }
  };

public:
  explicit any_result_submitter(T callback) : callback_(std::move(callback)) {
  }

  /// Creates a submitter which submits it's result to the callback
  auto create_callback() {
    return any_callback{this->shared_from_this()};
  }

private:
  // Invokes the callback with the given arguments
  template <typename... ActualArgs>
  void invoke(ActualArgs&&... args) {
    std::call_once(flag_, std::move(callback_),
                   std::forward<ActualArgs>(args)...);
  }
};

struct result_deducer {
  template <typename T>
  static auto deduce_one(std::false_type, traits::identity<T>) {
    static_assert(traits::fail<T>::value,
                  "Non continuable types except tuple like and homogeneous "
                  "containers aren't allowed inside an any expression!");
  }
  template <typename T>
  static auto deduce_one(std::true_type, traits::identity<T> id) {
    return hints::hint_of(id);
  }
  template <typename T>
  static auto deduce(traversal::container_category_tag<false, false>,
                     traits::identity<T> id) {
    return deduce_one<T>(base::is_continuable<T>{}, id);
  }

  /// Deduce a homogeneous container
  template <bool IsTupleLike, typename T>
  static auto deduce(traversal::container_category_tag<true, IsTupleLike>,
                     traits::identity<T>) {

    // Deduce the containing type
    using element_t = std::decay_t<decltype(*std::declval<T>().begin())>;
    return deduce(traversal::container_category_of_t<element_t>{},
                  traits::identity<element_t>{});
  }

  template <typename First, typename... T>
  static auto deduce_same_hints(First first, T...) {
    static_assert(traits::conjunction<std::is_same<First, T>...>::value,
                  "The continuables inside the given pack must have the "
                  "same signature hint!");

    return first;
  }

  template <std::size_t... I, typename T>
  static auto deduce_tuple_like(std::integer_sequence<std::size_t, I...>,
                                traits::identity<T>) {

    return deduce_same_hints(deduce(
        traversal::container_category_of_t<
            std::decay_t<decltype(std::get<I>(std::declval<T>()))>>{},
        traits::identity<
            std::decay_t<decltype(std::get<I>(std::declval<T>()))>>{})...);
  }

  /// Traverse tuple like container
  template <typename T>
  static auto deduce(traversal::container_category_tag<false, true>,
                     traits::identity<T> id) {

    constexpr auto const size = std::tuple_size<T>::value;
    return deduce_tuple_like(std::make_index_sequence<size>{}, id);
  }
};

template <typename Submitter>
struct continuable_dispatcher {
  std::shared_ptr<Submitter>& submitter;

  template <typename Continuable,
            std::enable_if_t<base::is_continuable<
                std::decay_t<Continuable>>::value>* = nullptr>
  void operator()(Continuable&& continuable) {
    // Retrieve a callback from the submitter and attach it to the continuable
    std::forward<Continuable>(continuable)
        .next(submitter->create_callback())
        .done();
  }
};
} // namespace any

struct composition_strategy_any_tag {};
template <>
struct is_composition_strategy<composition_strategy_any_tag> // ...
    : std::true_type {};

/// Finalizes the any logic of a given composition
template <>
struct composition_finalizer<composition_strategy_any_tag> {
  template <typename Composition>
  static auto finalize(Composition&& composition, util::ownership ownership) {
    auto signature = decltype(any::result_deducer::deduce(
        traversal::container_category_of_t<std::decay_t<Composition>>{},
        traits::identity<std::decay_t<Composition>>{})){};

    return base::attorney::create(
        [composition =
             std::forward<Composition>(composition)](auto&& callback) mutable {

          using submitter_t =
              any::any_result_submitter<std::decay_t<decltype(callback)>>;

          // Create the submitter which calls the given callback once at the
          // first callback invocation.
          auto submitter = std::make_shared<submitter_t>(
              std::forward<decltype(callback)>(callback));

          traverse_pack(any::continuable_dispatcher<submitter_t>{submitter},
                        std::move(composition));
        },
        signature, std::move(ownership));
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ANY_HPP_INCLUDED
