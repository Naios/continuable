
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

    template <typename... PartialArgs>
    void set_value(PartialArgs&&... args) {
      std::move (*this)(std::forward<PartialArgs>(args)...);
    }

    void set_exception(types::error_type error) {
      std::move (*this)(types::dispatch_error_tag{}, std::move(error));
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

/// Creates a submitter that continues `any` chains
template <typename Callback>
auto make_any_result_submitter(Callback&& callback) {
  return std::make_shared<
      any_result_submitter<std::decay_t<decltype(callback)>>>(
      std::forward<decltype(callback)>(callback));
}

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

    std::make_index_sequence<std::tuple_size<T>::value> constexpr const i{};
    return deduce_tuple_like(i, id);
  }
};
} // namespace any

/// Finalizes the any logic of a given composition
template <>
struct composition_finalizer<composition_strategy_any_tag> {
  template <typename Composition>
  static constexpr auto hint() {
    return decltype(any::result_deducer::deduce(
        traversal::container_category_of_t<std::decay_t<Composition>>{},
        traits::identity<std::decay_t<Composition>>{})){};
  }

  template <typename Composition>
  static auto finalize(Composition&& composition) {
    return [composition = std::forward<Composition>(composition)](
        auto&& callback) mutable {
      // Create the submitter which calls the given callback once at the
      // first callback invocation.
      auto submitter = any::make_any_result_submitter(
          std::forward<decltype(callback)>(callback));

      traits::static_for_each_in(std::move(composition),
                                 [&](auto&& entry) mutable {
                                   // Invoke the continuation with a
                                   // submission callback
                                   base::attorney::invoke_continuation(
                                       std::forward<decltype(entry)>(entry),
                                       submitter->create_callback());
                                 });
    };
  }
};

template <>
struct composition_finalizer<composition_strategy_any_fail_fast_tag>
    : composition_finalizer<composition_strategy_any_tag> {};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ANY_HPP_INCLUDED