
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

template <typename T>
struct check_pack_empty {
  static constexpr auto const is_empty =
      (decltype(traits::pack_size_of(std::declval<T>())){} ==
       traits::size_constant_of<0>());
  static_assert(is_empty.value, "Expected all continuations to have the same"
                                "count of arguments!");
};

/// A callable object to determine the shared result between all continuations
struct determine_shared_result {
  template <typename... T>
  constexpr auto operator()(T&... args) const noexcept {
    return common_result_of(hints::signature_hint_tag<>{},
                            hints::hint_of(traits::identity_of(args))...);
  }

private:
  template <typename Signature, typename... Args>
  static constexpr auto common_result_of(Signature signature,
                                         hints::signature_hint_tag<>,
                                         Args... /*args*/) {
    /// Assert that the other signatures are empty too which means all
    /// signatures had the same size.
    (void)std::initializer_list<int>{0, ((void)check_pack_empty<Args>{}, 0)...};
    return signature;
  }

  template <typename T, typename... Args>
  static constexpr T first_of(traits::identity<T, Args...>) noexcept;

  /// Determine the common result between all continuation which are chained
  /// with an `any` strategy, consider two continuations:
  /// c1 with `void(int)` and c2 with `void(float)`, the common result shared
  /// between both continuations is `void(int)`.
  template <typename Signature, typename First, typename... Args>
  static constexpr auto common_result_of(Signature signature, First first,
                                         Args... args) {
    using common_type =
        traits::identity<std::common_type_t<decltype(first_of(first)),
                                            decltype(first_of(args))...>>;

    return common_result_of(traits::push(signature, common_type{}),
                            traits::pop_first(first),
                            traits::pop_first(args)...);
  }
};
} // namespace any

/// Finalizes the any logic of a given composition
template <>
struct composition_finalizer<composition_strategy_any_tag> {
  template <typename Continuable>
  static auto finalize(Continuable&& continuation) {

    auto ownership = base::attorney::ownership_of(continuation);

    auto composition =
        base::attorney::consume_data(std::forward<Continuable>(continuation));

    constexpr auto const signature =
        traits::unpack(composition, any::determine_shared_result{});

    return base::attorney::create(
        [composition = std::move(composition)](auto&& callback) mutable {

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
        },
        signature, std::move(ownership));
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ANY_HPP_INCLUDED
