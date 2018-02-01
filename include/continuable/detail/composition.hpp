
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

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
/// The namespace `composition` offers methods to chain continuations together
/// with `all`, `any` or `seq` logic.
namespace composition {
struct strategy_all_tag {};
struct strategy_any_tag {};

template <typename T>
struct is_strategy : std::false_type {};
template <>
struct is_strategy<strategy_all_tag> : std::true_type {};
template <>
struct is_strategy<strategy_any_tag> : std::true_type {};

/// Provides support for extracting the signature hint out
/// of given types and parameters.
namespace annotating {
namespace detail {
/// Void hints are equal to an empty signature
constexpr auto make_hint_of(traits::identity<void>) noexcept {
  return hints::signature_hint_tag<>{};
}
/// All other hints are the obvious hints...
template <typename... HintArgs>
constexpr auto make_hint_of(traits::identity<HintArgs...> args) noexcept {
  return args; // Identity is equal to signature_hint_tag
}
} // namespace detail

/// Extracts the signature hint of a given continuation and it's optional
/// present hint arguments.
///
/// There are 3 cases:
/// - Any argument is given:
///   -> The hint is of the argument type where void is equal to no args
/// - An unwrappable type is given which first arguments signature is known
///   -> The hint is of the mentioned signature
/// - An object which signature isn't known
///   -> The hint is unknown
///
/// In any cases the return type is a:
/// - signature_hint_tag<?...> or a
/// - absent_signature_hint_tag
///
template <typename T, typename... HintArgs>
constexpr auto extract(traits::identity<T> /*type*/,
                       traits::identity<HintArgs...> hint) {
  return traits::static_if(hint, traits::is_empty(),
                           [=](auto /*hint*/) {
                             /// When the arguments are the hint is absent
                             return hints::absent_signature_hint_tag{};
                           },
                           [](auto hint) {
                             // When hint arguments are given just take it as
                             // hint
                             return detail::make_hint_of(hint);
                           });
}
} // namespace annotating

namespace detail {
template <std::size_t Pos, typename T>
constexpr void assign(traits::size_constant<Pos> /*pos*/, T& /*storage*/) {
  // ...
}
template <std::size_t Pos, typename T, typename Current, typename... Args>
void assign(traits::size_constant<Pos> pos, T& storage, Current&& current,
            Args&&... args) {
  // TODO Improve this -> linear instantiation
  std::get<Pos>(storage) = std::forward<Current>(current);
  assign(pos + traits::size_constant_of<1>(), storage,
         std::forward<Args>(args)...);
}

/// Caches the partial results and invokes the callback when all results
/// are arrived. This class is thread safe.
template <typename T, std::size_t Submissions, typename... Args>
class all_result_submitter : public std::enable_shared_from_this<
                                 all_result_submitter<T, Submissions, Args...>>,
                             public util::non_movable {

  T callback_;
  std::atomic<std::size_t> left_;
  std::once_flag flag_;
  std::tuple<Args...> result_;

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

  template <std::size_t From, std::size_t To>
  struct partial_all_callback {
    std::shared_ptr<all_result_submitter> me_;

    template <typename... PartialArgs>
    void operator()(PartialArgs&&... args) && {
      me_->resolve(traits::size_constant<From>{}, traits::size_constant<To>{},
                   std::forward<PartialArgs>(args)...);
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
  explicit all_result_submitter(T callback)
      : callback_(std::move(callback)), left_(Submissions) {
  }

  /// Creates a submitter which submits it's result into the tuple
  template <std::size_t From, std::size_t To>
  auto create_callback(traits::size_constant<From> /*from*/,
                       traits::size_constant<To> /*to*/) {
    return partial_all_callback<From, To>{this->shared_from_this()};
  }
};

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
} // namespace detail

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
template <typename Strategy, typename Data, typename Annotation,
          std::enable_if_t<!is_strategy<Annotation>::value>* = nullptr>
auto normalize(Strategy /*strategy*/,
               continuable_base<Data, Annotation>&& continuation) {

  // If the continuation isn't a strategy initialize the strategy
  return std::make_tuple(std::move(continuation));
}
/// - The continuable is in a different strategy then the current one:
///   -> materialize it
template <typename Strategy, typename Data, typename Annotation,
          std::enable_if_t<is_strategy<Annotation>::value>* = nullptr>
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

/// Creates a submitter which caches the intermediate results of `all` chains
template <typename Callback, std::size_t Submissions, typename... Args>
auto make_all_result_submitter(Callback&& callback,
                               traits::size_constant<Submissions>,
                               traits::identity<Args...>) {
  return std::make_shared<detail::all_result_submitter<
      std::decay_t<decltype(callback)>, Submissions, Args...>>(
      std::forward<decltype(callback)>(callback));
}

/// A callable object to merge multiple signature hints together
struct entry_merger {
  template <typename... T>
  constexpr auto operator()(T&... entries) const noexcept {
    return traits::merge(hints::hint_of(traits::identity_of(entries))...);
  }
};

/// Finalizes the all logic of a given composition
template <typename Data>
auto finalize_composition(
    continuable_base<Data, strategy_all_tag>&& continuation) {

  auto ownership_ = base::attorney::ownership_of(continuation);

  auto composition = base::attorney::consume_data(std::move(continuation));

  // Merge all signature hints together
  constexpr auto const signature = traits::unpack(composition, entry_merger{});

  return base::attorney::create(
      [ signature,
        composition = std::move(composition) ](auto&& callback) mutable {
        // We mark the current 2-dimensional position through a pair:
        // std::pair<size_constant<?>, size_constant<?>>
        //           ~~~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~
        //           Continuation pos     Result pos
        constexpr auto const begin = std::make_pair(
            traits::size_constant_of<0>(), traits::size_constant_of<0>());
        constexpr auto const pack = traits::identify<decltype(composition)>{};
        constexpr auto const end = traits::pack_size_of(pack);
        auto const condition = [=](auto pos) { return pos.first < end; };

        // Create the result submitter which caches all results and invokes
        // the final callback upon completion.
        auto submitter = make_all_result_submitter(
            std::forward<decltype(callback)>(callback), end, signature);

        // Invoke every continuation with it's callback of the submitter
        traits::static_while(begin, condition, [&](auto current) mutable {
          auto entry =
              std::move(std::get<decltype(current.first)::value>(composition));

          // This is the length of the arguments of the current continuable
          constexpr auto const arg_size =
              traits::pack_size_of(hints::hint_of(traits::identity_of(entry)));

          // The next position in the result tuple
          constexpr auto const next = current.second + arg_size;

          // Invoke the continuation with the associated submission callback
          base::attorney::invoke_continuation(
              std::move(entry),
              submitter->create_callback(current.second, next));

          return std::make_pair(current.first + traits::size_constant_of<1>(),
                                next);
        });
      },
      signature, std::move(ownership_));
}

/// Creates a submitter that continues `any` chains
template <typename Callback>
auto make_any_result_submitter(Callback&& callback) {
  return std::make_shared<
      detail::any_result_submitter<std::decay_t<decltype(callback)>>>(
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

/// Finalizes the any logic of a given composition
template <typename Data>
auto finalize_composition(
    continuable_base<Data, strategy_any_tag>&& continuation) {

  auto ownership = base::attorney::ownership_of(continuation);

  auto composition = base::attorney::consume_data(std::move(continuation));

  constexpr auto const signature =
      traits::unpack(composition, determine_shared_result{});

  return base::attorney::create(
      [composition = std::move(composition)](auto&& callback) mutable {

        // Create the submitter which calls the given callback once at the first
        // callback invocation.
        auto submitter = make_any_result_submitter(
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
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_HPP_INCLUDED
