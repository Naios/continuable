
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
#include <continuable/detail/composition.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace composition {
namespace all {
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

/// Creates a submitter which caches the intermediate results of `all` chains
template <typename Callback, std::size_t Submissions, typename... Args>
auto make_all_result_submitter(Callback&& callback,
                               traits::size_constant<Submissions>,
                               traits::identity<Args...>) {
  return std::make_shared<all_result_submitter<std::decay_t<decltype(callback)>,
                                               Submissions, Args...>>(
      std::forward<decltype(callback)>(callback));
}

/// A callable object to merge multiple signature hints together
struct entry_merger {
  template <typename... T>
  constexpr auto operator()(T&&...) const noexcept {
    return traits::merge(hints::hint_of(traits::identify<T>())...);
  }
};

struct all_hint_deducer {
  static constexpr auto deduce(hints::signature_hint_tag<>) noexcept {
    return spread_this();
  }
  template <typename First>
  static constexpr auto deduce(hints::signature_hint_tag<First>) {
    return First{};
  }
  template <typename First, typename Second, typename... Args>
  static constexpr auto
  deduce(hints::signature_hint_tag<First, Second, Args...>) {
    return spread_this(First{}, Second{}, Args{}...);
  }

  template <
      typename T,
      std::enable_if_t<base::is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& /*continuable*/) const {
    return deduce(hints::hint_of(traits::identify<T>{}));
  }
};

template <typename T>
struct is_tuple : std::false_type {};
template <typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type {};

/// Converts the given argument to a tuple if it isn't a tuple already
template <typename T, std::enable_if_t<!is_tuple<T>::value>* = nullptr>
constexpr auto tupelize(T&& arg) {
  return std::make_tuple(std::forward<T>(arg));
}
/// Converts the given argument to a tuple if it isn't a tuple already
template <typename... T>
constexpr auto tupelize(std::tuple<T...> arg) {
  return std::move(arg);
}

/// Lift the first tuple hierarchy inside the given tuple
template <typename T>
constexpr auto flatten(T&& tuple) {
  return traits::unpack(std::forward<T>(tuple), [](auto&&... args) {
    return std::tuple_cat(tupelize(std::forward<decltype(args)>(args))...);
  });
}

constexpr auto deduce_from_pack(traits::identity<void>)
    -> hints::signature_hint_tag<>;
template <typename... T>
constexpr auto deduce_from_pack(traits::identity<std::tuple<T...>>)
    -> hints::signature_hint_tag<T...>;
template <typename T, std::enable_if_t<!is_tuple<T>::value>* = nullptr>
constexpr auto deduce_from_pack(traits::identity<T>)
    -> hints::signature_hint_tag<T>;

template <typename Composition>
constexpr auto deduce_hint(Composition&& /*composition*/) {
  using deduced_t =
      decltype(map_pack(all_hint_deducer{}, std::declval<Composition>()));

  // We must guard deduced_t against to be void since this represents an
  // empty signature hint.
  return decltype(deduce_from_pack(traits::identity<deduced_t>{})){};
}
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

      // We mark the current 2-dimensional position through a pair:
      // std::pair<size_constant<?>, size_constant<?>>
      //           ~~~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~
      //           Continuation pos     Result pos
      constexpr auto const begin = std::make_pair(
          traits::size_constant_of<0>(), traits::size_constant_of<0>());
      constexpr auto const pack = traits::identify<decltype(composition)>{};
      constexpr auto const end = traits::pack_size_of(pack);
      auto const condition = [=](auto pos) { return pos.first < end; };

      constexpr auto const signature = hint<Composition>();

      // Create the result submitter which caches all results and invokes
      // the final callback upon completion.
      auto submitter = all::make_all_result_submitter(
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
            std::move(entry), submitter->create_callback(current.second, next));

        return std::make_pair(current.first + traits::size_constant_of<1>(),
                              next);
      });
    };
  }
};
} // namespace composition
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_COMPOSITION_ALL_HPP_INCLUDED
