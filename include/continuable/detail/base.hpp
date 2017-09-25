
/**

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2017 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_DETAIL_BASE_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_BASE_HPP_INCLUDED__

#include <tuple>
#include <type_traits>
#include <utility>

#include "continuable/detail/api.hpp"
#include "continuable/detail/hints.hpp"
#include "continuable/detail/traits.hpp"
#include "continuable/detail/util.hpp"

namespace cti {
namespace detail {
/// The namespace `base` provides the low level API for working
/// with continuable types.
///
/// Important methods are:
/// - Creating a continuation from a callback taking functional
///   base::attorney::create(auto&& callback)
///     -> base::continuation<auto>
/// - Chaining a continuation together with a callback
///   base::chain_continuation(base::continuation<auto> continuation,
///                            auto&& callback)
///     -> base::continuation<auto>
/// - Finally invoking the continuation chain
///    base::finalize_continuation(base::continuation<auto> continuation)
///     -> void
namespace base {
struct this_thread_executor_tag {};

/// Returns the signature hint of the given continuable
template <typename T>
constexpr auto hint_of(util::identity<T>) {
  static_assert(util::fail<T>::value,
                "Expected a continuation with an existing signature hint!");
  return util::identity_of<void>();
}
/// Returns the signature hint of the given continuable
template <typename Data, typename... Args>
constexpr auto
hint_of(util::identity<
        continuable_base<Data, hints::signature_hint_tag<Args...>>>) {
  return hints::signature_hint_tag<Args...>{};
}

template <typename T>
struct is_continuation : std::false_type {};
template <typename Data, typename Annotation>
struct is_continuation<continuable_base<Data, Annotation>> : std::true_type {};

/// Helper class to access private methods and members of
/// the continuable_base class.
struct attorney {
  /// Makes a continuation wrapper from the given argument
  template <typename T, typename A = hints::absent_signature_hint_tag>
  static auto create(T&& continuation, A /*hint*/, util::ownership ownership_) {
    return continuable_base<std::decay_t<T>, std::decay_t<A>>(
        std::forward<T>(continuation), ownership_);
  }

  /// Invokes a continuation object in a reference correct way
  template <typename Data, typename Annotation, typename Callback>
  static auto
  invoke_continuation(continuable_base<Data, Annotation>&& continuation,
                      Callback&& callback) {
    auto materialized = std::move(continuation).materialize();
    materialized.release();
    return materialized.data_(std::forward<Callback>(callback));
  }

  template <typename Data, typename Annotation>
  static auto materialize(continuable_base<Data, Annotation>&& continuation) {
    return std::move(continuation).materialize();
  }

  template <typename Data, typename Annotation>
  static Data&&
  consume_data(continuable_base<Data, Annotation>&& continuation) {
    return std::move(continuation).consume_data();
  }

  template <typename Continuable>
  static util::ownership ownership_of(Continuable&& continuation) {
    return continuation.ownership_;
  }
};

// Returns the invoker of a callback, the next callback
// and the arguments of the previous continuation.
//
// The return type of the invokerOf function matches a functional of:
//   void(auto&& callback, auto&& nextCallback, auto&&... args)
//
// The invoker decorates the result type in the following way
// - void              -> nextCallback()
// - ?                 -> nextCallback(?)
// - std::pair<?, ?>   -> nextCallback(?, ?)
// - std::tuple<?...>  -> nextCallback(?...)
//
// When the result is a continuation itself pass the callback to it
// - continuation<?...> -> result(nextCallback);
namespace decoration {
/// Helper class wrapping the underlaying unwrapping lambda
/// in order to extend it with a hint method.
template <typename T, typename Hint>
class invoker : public T {
public:
  explicit invoker(T invoke) : T(std::move(invoke)) {
  }

  using T::operator();

  /// Returns the underlaying signature hint
  static constexpr Hint hint() noexcept {
    return {};
  }
};

template <typename T, typename... Args>
constexpr auto make_invoker(T&& invoke, hints::signature_hint_tag<Args...>) {
  return invoker<std::decay_t<T>, hints::signature_hint_tag<Args...>>(
      std::forward<T>(invoke));
}

/// - continuable<?...> -> result(nextCallback);
template <typename Data, typename Annotation>
constexpr auto invoker_of(util::identity<continuable_base<Data, Annotation>>) {
  /// Get the hint of the unwrapped returned continuable
  using Type = decltype(attorney::materialize(
      std::declval<continuable_base<Data, Annotation>>()));

  return make_invoker(
      [](auto&& callback, auto&& nextCallback, auto&&... args) {
        auto continuation_ = std::forward<decltype(callback)>(callback)(
            std::forward<decltype(args)>(args)...);

        attorney::invoke_continuation(
            std::move(continuation_),
            std::forward<decltype(nextCallback)>(nextCallback));
      },
      hint_of(util::identity_of<Type>()));
}

/// - ? -> nextCallback(?)
template <typename T>
auto invoker_of(util::identity<T>) {
  return make_invoker(
      [](auto&& callback, auto&& nextCallback, auto&&... args) {
        auto result = std::forward<decltype(callback)>(callback)(
            std::forward<decltype(args)>(args)...);

        std::forward<decltype(nextCallback)>(nextCallback)(std::move(result));
      },
      util::identity_of<T>());
}

/// - void -> nextCallback()
inline auto invoker_of(util::identity<void>) {
  return make_invoker(
      [](auto&& callback, auto&& nextCallback, auto&&... args) {
        std::forward<decltype(callback)>(callback)(
            std::forward<decltype(args)>(args)...);

        std::forward<decltype(nextCallback)>(nextCallback)();
      },
      util::identity<>{});
}

/// Returns a sequenced invoker which is able to invoke
/// objects where std::get is applicable.
inline auto sequenced_unpack_invoker() {
  return [](auto&& callback, auto&& nextCallback, auto&&... args) {
    auto result = std::forward<decltype(callback)>(callback)(
        std::forward<decltype(args)>(args)...);

    util::unpack(std::move(result), [&](auto&&... types) {
      /// TODO Add inplace resolution here

      std::forward<decltype(nextCallback)>(nextCallback)(
          std::forward<decltype(types)>(types)...);
    });
  };
}

// - std::pair<?, ?> -> nextCallback(?, ?)
template <typename First, typename Second>
constexpr auto invoker_of(util::identity<std::pair<First, Second>>) {
  return make_invoker(sequenced_unpack_invoker(),
                      util::identity<First, Second>{});
}

// - std::tuple<?...>  -> nextCallback(?...)
template <typename... Args>
constexpr auto invoker_of(util::identity<std::tuple<Args...>>) {
  return make_invoker(sequenced_unpack_invoker(), util::identity<Args...>{});
}
} // end namespace decoration

/// Invoke the callback immediately
template <typename Invoker, typename Callback, typename NextCallback,
          typename... Args>
void packed_dispatch(this_thread_executor_tag, Invoker&& invoker,
                     Callback&& callback, NextCallback&& nextCallback,
                     Args&&... args) {

  // Invoke the callback with the decorated invoker immediately
  std::forward<Invoker>(invoker)(std::forward<Callback>(callback),
                                 std::forward<NextCallback>(nextCallback),
                                 std::forward<Args>(args)...);
}

/// Invoke the callback through the given executor
template <typename Executor, typename Invoker, typename Callback,
          typename NextCallback, typename... Args>
void packed_dispatch(Executor&& executor, Invoker&& invoker,
                     Callback&& callback, NextCallback&& nextCallback,
                     Args&&... args) {

  // Create a worker object which when invoked calls the callback with the
  // the returned arguments.
  auto work = [
    invoker = std::forward<Invoker>(invoker),
    callback = std::forward<Callback>(callback),
    nextCallback = std::forward<NextCallback>(nextCallback),
    args = std::make_tuple(std::forward<Args>(args)...)
  ]() mutable {
    util::unpack(std::move(args), [&](auto&&... captured_args) {
      // Just use the packed dispatch method which dispatches the work on
      // the current thread.
      packed_dispatch(this_thread_executor_tag{}, std::move(invoker),
                      std::move(callback), std::move(nextCallback),
                      std::forward<decltype(captured_args)>(captured_args)...);
    });
  };

  // Pass the work functional object to the executor
  std::forward<Executor>(executor)(std::move(work));
}

/// Invokes a continuation with a given callback.
/// Passes the next callback to the resulting continuable or
/// invokes the next callback directly if possible.
///
/// For example given:
/// - Continuation: continuation<[](auto&& callback) { callback("hi"); }>
/// - Callback: [](std::string) { }
/// - NextCallback: []() { }
///
template <typename... Args, typename Continuation, typename Callback,
          typename Executor, typename NextCallback>
void invoke_proxy(hints::signature_hint_tag<Args...>,
                  Continuation&& continuation, Callback&& callback,
                  Executor&& executor, NextCallback&& nextCallback) {

  // Invoke the continuation with a proxy callback.
  // The proxy callback is responsible for passing the
  // the result to the callback as well as decorating it.
  attorney::invoke_continuation(std::forward<Continuation>(continuation), [
    callback = std::forward<Callback>(callback),
    executor = std::forward<Executor>(executor),
    nextCallback = std::forward<NextCallback>(nextCallback)
  ](Args... args) mutable {

    // In order to retrieve the correct decorator we must know what the
    // result type is.
    auto result =
        util::identity_of<decltype(std::move(callback)(std::move(args)...))>();

    // Pick the correct invoker that handles decorating of the result
    auto invoker = decoration::invoker_of(result);

    // Invoke the callback
    packed_dispatch(std::move(executor), std::move(invoker),
                    std::move(callback), std::move(nextCallback),
                    std::move(args)...);
  });
}

/// Returns the next hint when the callback is invoked with the given hint
template <typename T, typename... Args>
constexpr auto next_hint_of(util::identity<T> /*callback*/,
                            hints::signature_hint_tag<Args...> /*current*/) {
  return decoration::invoker_of(util::identity_of<decltype(std::declval<T>()(
                                    std::declval<Args>()...))>())
      .hint();
}

/// Chains a callback together with a continuation and returns a continuation:
///
/// For example given:
/// - Continuation: continuation<[](auto&& callback) { callback("hi"); }>
/// - Callback: [](std::string) { }
///
/// This function returns a function accepting the next callback in the chain:
/// - Result: continuation<[](auto&& callback) { /*...*/ }>
///
template <typename Continuation, typename Callback, typename Executor>
auto chain_continuation(Continuation&& continuation, Callback&& callback,
                        Executor&& executor) {
  static_assert(is_continuation<std::decay_t<Continuation>>{},
                "Expected a continuation!");

  // Wrap the callback into a partial callable callback
  auto partial_callable = [callback = std::forward<Callback>(callback)](
      auto&&... args) mutable {
    return util::partial_invoke(std::move(callback),
                                std::forward<decltype(args)>(args)...);
  };

  auto hint = hint_of(util::identity_of(continuation));
  auto next_hint = next_hint_of(util::identity_of(partial_callable), hint);

  auto ownership_ = attorney::ownership_of(continuation);
  continuation.freeze();

  return attorney::create(
      [
        // TODO consume only the data here
        continuation = std::forward<Continuation>(continuation),
        partial_callable = std::move(partial_callable),
        executor = std::forward<Executor>(executor)
      ](auto&& nextCallback) mutable {
        invoke_proxy(hint_of(util::identity_of(continuation)),
                     std::move(continuation), std::move(partial_callable),
                     std::move(executor),
                     std::forward<decltype(nextCallback)>(nextCallback));
      },
      next_hint, ownership_);
}

/// Workaround for GCC bug:
/// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
struct empty_callback {
  template <typename... Args>
  void operator()(Args...) const {
  }
};

/// Final invokes the given continuation chain:
///
/// For example given:
/// - Continuation: continuation<[](auto&& callback) { callback("hi"); }>
template <typename Continuation>
void finalize_continuation(Continuation&& continuation) {
  attorney::invoke_continuation(std::forward<Continuation>(continuation),
                                empty_callback{});
}

/// Workaround for GCC bug:
/// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
template <typename T>
class supplier_callback {
  T data_;

public:
  explicit supplier_callback(T data) : data_(std::move(data)) {
  }

  template <typename... Args>
  auto operator()(Args...) {
    return std::move(data_);
  }
};

/// Returns a continuable into a functional object returning the continuable
template <typename Continuation>
auto wrap_continuation(Continuation&& continuation) {
  continuation.freeze();
  return supplier_callback<std::decay_t<Continuation>>(
      std::forward<Continuation>(continuation));
}
} // end namespace base

} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_BASE_HPP_INCLUDED__
