
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
  static constexpr auto deduce(hints::signature_hint_tag<>) noexcept
      -> decltype(spread_this());

  template <typename First>
  static constexpr auto deduce(hints::signature_hint_tag<First>) -> First;

  template <typename First, typename Second, typename... Args>
  static constexpr auto
  deduce(hints::signature_hint_tag<First, Second, Args...>)
      -> decltype(spread_this(std::declval<First>(), std::declval<Second>(),
                              std::declval<Args>()...));

  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& /*continuable*/) const
      -> decltype(deduce(hints::hint_of(traits::identify<T>{})));
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
constexpr auto deduce_hint(Composition && /*composition*/)
    -> decltype(deduce_from_pack(
        traits::identity<decltype(map_pack(all_hint_deducer{},
                                           std::declval<Composition>()))>{})){};

/// Caches the partial results and invokes the callback when all results
/// are arrived. This class is thread safe.
template <typename Callback, typename Result>
class all_result_submitter : public std::enable_shared_from_this<
                                 all_result_submitter<Callback, Result>>,
                             public util::non_movable {

  Callback callback_;
  Result result_;

  std::atomic<std::size_t> left_;
  std::once_flag flag_;

  template <std::size_t From, std::size_t To, typename... PartialArgs>
  void resolve(traits::size_constant<From> from, traits::size_constant<To>,
               PartialArgs&&... args) {

    static_assert(sizeof...(args) == (To - From),
                  "Submission called with the wrong amount of arguments!");

    // Assign the values from the result to it's correct positions of the
    // tuple. Maybe think about the thread safety again...:
    // http://stackoverflow.com/questions/40845699
    assign(from, result_, std::forward<PartialArgs>(args)...);

    // Complete the current result
    complete_one();
  }

  template <std::size_t From, std::size_t To>
  void resolve(traits::size_constant<From>, traits::size_constant<To>,
               types::dispatch_error_tag tag, types::error_type error) {

    // We never complete the composition, but we forward the first error
    // which was raised.
    std::call_once(flag_, std::move(callback_), tag, std::move(error));
  }

  // Invokes the callback with the cached result
  void invoke() {
    assert((left_ == 0U) && "Expected that the submitter is finished!");
    std::atomic_thread_fence(std::memory_order_acquire);
    traits::unpack(std::move(result_), [&](auto&&... args) {
      std::call_once(flag_, std::move(callback_),
                     std::forward<decltype(args)>(args)...);
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

  template <typename Target>
  struct partial_all_callback {
    Target* target_;
    std::shared_ptr<all_result_submitter> me_;

    template <typename... PartialArgs>
    void operator()(PartialArgs&&... args) && {
    }

    template <typename... PartialArgs>
    void operator()(types::dispatch_error_tag, types::error_type) && {
    }
  };

public:
  explicit all_result_submitter(Callback callback, Result&& result)
      : callback_(std::move(callback)), result_(std::move(result)), left_(1) {
  }

  /// Creates a submitter which submits it's result into the storage
  template <typename Target>
  auto create_callback(Target* target) {
    left_.fetch_add(1, std::memory_order_seq_cst);
    return partial_all_callback<std::decay_t<Target>>{target,
                                                      this->shared_from_this()};
  }

  /// Initially the counter is created with an initial count of 1 in order
  /// to prevent that the composition is finished before all callbacks
  /// were registered.
  void start_accept() {
  }
};

struct continuable_dispatcher {
  template <typename Index, typename Target,
            std::enable_if_t<
                base::is_continuable<std::decay_t<Index>>::value>* = nullptr>
  auto operator()(Index* index, Target* target) const noexcept {
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
    return [composition = std::forward<Composition>(composition)](
        auto&& callback) mutable {

      auto result =
          remapping::create_result_pack(std::forward<Composition>(composition));
    };
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ALL_HPP_INCLUDED
