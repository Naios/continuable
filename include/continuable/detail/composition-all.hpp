
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

#ifndef CONTINUABLE_DETAIL_COMPOSITION_ALL_HPP_INCLUDED
#define CONTINUABLE_DETAIL_COMPOSITION_ALL_HPP_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/detail/base.hpp>
#include <continuable/detail/composition-remapping.hpp>
#include <continuable/detail/composition.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace composition {
namespace all {
struct all_hint_deducer {
  static constexpr auto deduce(hints::signature_hint_tag<>) noexcept {
    return spread_this();
  }

  template <typename First>
  static constexpr auto deduce(hints::signature_hint_tag<First>) {
    return std::declval<First>();
  }

  template <typename First, typename Second, typename... Args>
  static constexpr auto
  deduce(hints::signature_hint_tag<First, Second, Args...>) {
    return spread_this(std::declval<First>(), std::declval<Second>(),
                       std::declval<Args>()...);
  }

  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& /*continuable*/) const {
    return deduce(hints::hint_of(traits::identify<T>{}));
  }
};

constexpr auto deduce_from_pack(traits::identity<void>)
    -> hints::signature_hint_tag<>;
template <typename... T>
constexpr auto deduce_from_pack(traits::identity<std::tuple<T...>>)
    -> hints::signature_hint_tag<T...>;
template <typename T>
constexpr auto deduce_from_pack(traits::identity<T>)
    -> hints::signature_hint_tag<T>;

// We must guard the mapped type against to be void since this represents an
// empty signature hint.
template <typename Composition>
constexpr auto deduce_hint(Composition&& /*composition*/) {
  // Don't change this way since it addresses a GCC compiler bug:
  // error: extra ';' [-Werror=pedantic]
  //  std::declval<Composition>()))>{})){};
  using mapped_t =
      decltype(map_pack(all_hint_deducer{}, std::declval<Composition>()));
  using deduced_t = decltype(deduce_from_pack(traits::identity<mapped_t>{}));
  return deduced_t{};
}

/// Caches the partial results and invokes the callback when all results
/// are arrived. This class is thread safe.
template <typename Callback, typename Result>
class result_submitter
    : public std::enable_shared_from_this<result_submitter<Callback, Result>>,
      public util::non_movable {

  Callback callback_;
  Result result_;

  std::atomic<std::size_t> left_;
  std::once_flag flag_;

  // Invokes the callback with the cached result
  void invoke() {
    assert((left_ == 0U) && "Expected that the submitter is finished!");
    std::atomic_thread_fence(std::memory_order_acquire);

    // Call the final callback with the cleaned result
    std::call_once(flag_, [&] {
      remapping::finalize_data(std::move(callback_), std::move(result_));
    });
  }

  // Completes one result
  void complete_one() {
    assert((left_ > 0U) && "Expected that the submitter isn't finished!");

    auto const current = --left_;
    if (!current) {
      invoke();
    }
  }

  template <typename Box>
  struct partial_all_callback {
    Box* box;
    std::shared_ptr<result_submitter> me;

    template <typename... Args>
    void operator()(Args&&... args) && {

      // Assign the result to the target
      box->assign(std::forward<decltype(args)>(args)...);

      // Complete one result
      me->complete_one();
    }

    template <typename... PartialArgs>
    void operator()(types::dispatch_error_tag tag, types::error_type error) && {
      // We never complete the composition, but we forward the first error
      // which was raised.
      std::call_once(me->flag_, std::move(me->callback_), tag,
                     std::move(error));
    }
  };

public:
  explicit result_submitter(Callback callback, Result&& result)
      : callback_(std::move(callback)), result_(std::move(result)), left_(1) {
  }

  /// Creates a submitter which submits it's result into the storage
  template <typename Box>
  auto create_callback(Box* box) {
    left_.fetch_add(1, std::memory_order_seq_cst);
    return partial_all_callback<std::decay_t<Box>>{box,
                                                   this->shared_from_this()};
  }

  /// Initially the counter is created with an initial count of 1 in order
  /// to prevent that the composition is finished before all callbacks
  /// were registered.
  void accept() {
    complete_one();
  }

  constexpr auto& head() noexcept {
    return result_;
  }
};

template <typename Submitter>
struct continuable_dispatcher {
  std::shared_ptr<Submitter>& submitter;

  template <typename Box, std::enable_if_t<remapping::is_continuable_box<
                              std::decay_t<Box>>::value>* = nullptr>
  void operator()(Box&& box) const {
    // Retrieve a callback from the submitter and attach it to the continuable
    box.fetch().next(submitter->create_callback(std::addressof(box))).done();
  }
};
} // namespace all

/// Finalizes the all logic of a given composition
template <>
struct composition_finalizer<composition_strategy_all_tag> {
  template <typename Composition>
  static constexpr auto hint() {
    return decltype(all::deduce_hint(std::declval<Composition>())){};
  }

  /// Finalizes the all logic of a given composition
  template <typename Composition>
  static auto finalize(Composition&& composition) {
    return [composition = std::forward<Composition>(composition)] // ...
        (auto&& callback) mutable {

      // Create the target result from the composition
      auto result = remapping::box_continuables(std::move(composition));

      using submitter_t =
          all::result_submitter<std::decay_t<decltype(callback)>,
                                std::decay_t<decltype(result)>>;

      // Create the shared state which holds the result and the final callback
      auto state = std::make_shared<submitter_t>(
          std::forward<decltype(callback)>(callback), std::move(result));

      // Dispatch the continuables and store its partial result
      // in the whole result
      traverse_pack(all::continuable_dispatcher<submitter_t>{state},
                    state->head());

      // Finalize the composition if all results arrived in-place
      state->accept();
    };
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ALL_HPP_INCLUDED
