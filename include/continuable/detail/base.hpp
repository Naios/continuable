
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

#ifndef CONTINUABLE_DETAIL_BASE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_BASE_HPP_INCLUDED

#include <tuple>
#include <type_traits>
#include <utility>

#include <continuable/detail/features.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

#if defined(CONTINUABLE_WITH_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_WITH_EXCEPTIONS

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
                      Callback&& callback) noexcept {
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
//   void(auto&& callback, auto&& next_callback, auto&&... args)
//
// The invoker decorates the result type in the following way
// - void              -> next_callback()
// - ?                 -> next_callback(?)
// - std::pair<?, ?>   -> next_callback(?, ?)
// - std::tuple<?...>  -> next_callback(?...)
//
// When the result is a continuation itself pass the callback to it
// - continuation<?...> -> result(next_callback);
namespace decoration {
/// Helper class wrapping the underlaying unwrapping lambda
/// in order to extend it with a hint method.
template <typename T, typename Hint>
class invoker : public T {
public:
  constexpr explicit invoker(T invoke) : T(std::move(invoke)) {
  }

  using T::operator();

  /// Returns the underlaying signature hint
  static constexpr Hint hint() noexcept {
    return {};
  }
};

#if defined(CONTINUABLE_WITH_EXCEPTIONS)
#define CONTINUABLE_BLOCK_TRY_BEGIN try {
#define CONTINUABLE_BLOCK_TRY_END                                              \
  }                                                                            \
  catch (...) {                                                                \
    std::forward<decltype(next_callback)>(next_callback)(                      \
        types::dispatch_error_tag{}, std::current_exception());                \
  }

#else // CONTINUABLE_WITH_EXCEPTIONS
#define CONTINUABLE_BLOCK_TRY_BEGIN {
#define CONTINUABLE_BLOCK_TRY_END }
#endif // CONTINUABLE_WITH_EXCEPTIONS

/// Invokes the given callable object with the given arguments while
/// marking the operation as non exceptional.
template <typename T, typename... Args>
constexpr auto invoke_no_except(T&& callable, Args&&... args) noexcept {
  return std::forward<T>(callable)(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
constexpr auto make_invoker(T&& invoke, hints::signature_hint_tag<Args...>) {
  return invoker<std::decay_t<T>, hints::signature_hint_tag<Args...>>(
      std::forward<T>(invoke));
}

/// - continuable<?...> -> result(next_callback);
template <typename Data, typename Annotation>
constexpr auto
invoker_of(traits::identity<continuable_base<Data, Annotation>>) {
  /// Get the hint of the unwrapped returned continuable
  using Type = decltype(attorney::materialize(
      std::declval<continuable_base<Data, Annotation>>()));

  auto constexpr const hint = hints::hint_of(traits::identify<Type>{});

  return make_invoker(
      [](auto&& callback, auto&& next_callback, auto&&... args) {
        CONTINUABLE_BLOCK_TRY_BEGIN
          auto continuation_ =
              util::partial_invoke(std::forward<decltype(callback)>(callback),
                                   std::forward<decltype(args)>(args)...);

          attorney::invoke_continuation(
              std::move(continuation_),
              std::forward<decltype(next_callback)>(next_callback));
        CONTINUABLE_BLOCK_TRY_END
      },
      hint);
}

/// - ? -> next_callback(?)
template <typename T>
constexpr auto invoker_of(traits::identity<T>) {
  return make_invoker(
      [](auto&& callback, auto&& next_callback, auto&&... args) {
        CONTINUABLE_BLOCK_TRY_BEGIN
          auto result =
              util::partial_invoke(std::forward<decltype(callback)>(callback),
                                   std::forward<decltype(args)>(args)...);

          invoke_no_except(std::forward<decltype(next_callback)>(next_callback),
                           std::move(result));
        CONTINUABLE_BLOCK_TRY_END
      },
      traits::identify<T>{});
}

/// - void -> next_callback()
inline auto invoker_of(traits::identity<void>) {
  return make_invoker(
      [](auto&& callback, auto&& next_callback, auto&&... args) {
        CONTINUABLE_BLOCK_TRY_BEGIN
          util::partial_invoke(std::forward<decltype(callback)>(callback),
                               std::forward<decltype(args)>(args)...);
          invoke_no_except(
              std::forward<decltype(next_callback)>(next_callback));
        CONTINUABLE_BLOCK_TRY_END
      },
      traits::identity<>{});
}

/// Returns a sequenced invoker which is able to invoke
/// objects where std::get is applicable.
inline auto sequenced_unpack_invoker() {
  return [](auto&& callback, auto&& next_callback, auto&&... args) {
    CONTINUABLE_BLOCK_TRY_BEGIN
      auto result =
          util::partial_invoke(std::forward<decltype(callback)>(callback),
                               std::forward<decltype(args)>(args)...);

      // Workaround for MSVC not capturing the reference correctly inside
      // the lambda.
      using Next = decltype(next_callback);

      traits::unpack(std::move(result), [&](auto&&... types) {
        /// TODO Add inplace resolution here

        invoke_no_except(std::forward<Next>(next_callback),
                         std::forward<decltype(types)>(types)...);
      });
    CONTINUABLE_BLOCK_TRY_END
  };
} // namespace decoration

// - std::pair<?, ?> -> next_callback(?, ?)
template <typename First, typename Second>
constexpr auto invoker_of(traits::identity<std::pair<First, Second>>) {
  return make_invoker(sequenced_unpack_invoker(),
                      traits::identity<First, Second>{});
}

// - std::tuple<?...>  -> next_callback(?...)
template <typename... Args>
constexpr auto invoker_of(traits::identity<std::tuple<Args...>>) {
  return make_invoker(sequenced_unpack_invoker(), traits::identity<Args...>{});
}

#undef CONTINUABLE_BLOCK_TRY_BEGIN
#undef CONTINUABLE_BLOCK_TRY_END
} // namespace decoration

/// Invoke the callback immediately
template <typename Invoker, typename... Args>
void packed_dispatch(types::this_thread_executor_tag, Invoker&& invoker,
                     Args&&... args) {

  // Invoke the callback with the decorated invoker immediately
  std::forward<Invoker>(invoker)(std::forward<Args>(args)...);
}

/// Invoke the callback through the given executor
template <typename Executor, typename Invoker, typename... Args>
void packed_dispatch(Executor&& executor, Invoker&& invoker, Args&&... args) {

  // Create a worker object which when invoked calls the callback with the
  // the returned arguments.
  auto work = [
    invoker = std::forward<Invoker>(invoker),
    args = std::make_tuple(std::forward<Args>(args)...)
  ]() mutable {
    traits::unpack(std::move(args), [&](auto&&... captured_args) {
      // Just use the packed dispatch method which dispatches the work on
      // the current thread.
      packed_dispatch(types::this_thread_executor_tag{}, std::move(invoker),
                      std::forward<decltype(captured_args)>(captured_args)...);
    });
  };

  // Pass the work functional object to the executor
  std::forward<Executor>(executor)(std::move(work));
}

/// Tells whether we potentially move the chain upwards and handle the result
enum class handle_results {
  no, //< The result is forwarded to the next callable
  yes //< The result is handled by the current callable
};

/// Tells whether we handle the error through the callback
enum class handle_errors {
  no,     //< The error is forwarded to the next callable
  plain,  //< The error is the only argument accepted by the callable
  forward //< The error is forwarded to the callable while keeping its tag
};

namespace callbacks {
namespace proto {
template <handle_results HandleResults, typename Base, typename Hint>
struct result_handler_base;
template <typename Base, typename... Args>
struct result_handler_base<handle_results::no, Base,
                           hints::signature_hint_tag<Args...>> {
  void operator()(Args... args) && {
    // Forward the arguments to the next callback
    std::move(static_cast<Base*>(this)->next_callback_)(std::move(args)...);
  }
};
template <typename Base, typename... Args>
struct result_handler_base<handle_results::yes, Base,
                           hints::signature_hint_tag<Args...>> {
  /// The operator which is called when the result was provided
  void operator()(Args... args) && {
    // In order to retrieve the correct decorator we must know what the
    // result type is.
    auto result = traits::identify<decltype(util::partial_invoke(
        std::move(static_cast<Base*>(this)->callback_), std::move(args)...))>{};

    // Pick the correct invoker that handles decorating of the result
    auto invoker = decoration::invoker_of(result);

    // Invoke the callback
    packed_dispatch(std::move(static_cast<Base*>(this)->executor_),
                    std::move(invoker),
                    std::move(static_cast<Base*>(this)->callback_),
                    std::move(static_cast<Base*>(this)->next_callback_),
                    std::move(args)...);
  }
};

inline auto make_error_invoker(
    std::integral_constant<handle_errors, handle_errors::plain>) noexcept {
  return [](auto&& callback, types::error_type&& error) {
    // Errors are not partial invoked
    std::move(callback)(std::move(error));
  };
}
inline auto make_error_invoker(
    std::integral_constant<handle_errors, handle_errors::forward>) noexcept {
  return [](auto&& callback, types::error_type&& error) {
    // Errors are not partial invoked
    std::move(callback)(types::dispatch_error_tag{}, std::move(error));
  };
}

template <handle_errors HandleErrors /* = plain or forward*/, typename Base>
struct error_handler_base {
  void operator()(types::dispatch_error_tag, types::error_type error) && {
    // Just invoke the error handler, cancel the calling hierarchy after
    auto invoker = make_error_invoker(
        std::integral_constant<handle_errors, HandleErrors>{});

    // Invoke the error handler
    packed_dispatch(
        std::move(static_cast<Base*>(this)->executor_), std::move(invoker),
        std::move(static_cast<Base*>(this)->callback_), std::move(error));
  }
};
template <typename Base>
struct error_handler_base<handle_errors::no, Base> {
  /// The operator which is called when an error occurred
  void operator()(types::dispatch_error_tag tag, types::error_type error) && {
    // Forward the error to the next callback
    std::move(static_cast<Base*>(this)->next_callback_)(tag, std::move(error));
  }
};
} // namespace proto

template <typename Hint, handle_results HandleResults,
          handle_errors HandleErrors, typename Callback, typename Executor,
          typename NextCallback>
struct callback_base;

template <typename... Args, handle_results HandleResults,
          handle_errors HandleErrors, typename Callback, typename Executor,
          typename NextCallback>
struct callback_base<hints::signature_hint_tag<Args...>, HandleResults,
                     HandleErrors, Callback, Executor, NextCallback>
    : proto::result_handler_base<
          HandleResults,
          callback_base<hints::signature_hint_tag<Args...>, HandleResults,
                        HandleErrors, Callback, Executor, NextCallback>,
          hints::signature_hint_tag<Args...>>,
      proto::error_handler_base<
          HandleErrors,
          callback_base<hints::signature_hint_tag<Args...>, HandleResults,
                        HandleErrors, Callback, Executor, NextCallback>> {

  using CallbackT = Callback;

  Callback callback_;
  Executor executor_;
  NextCallback next_callback_;

  explicit callback_base(Callback callback, Executor executor,
                         NextCallback next_callback)
      : callback_(std::move(callback)), executor_(std::move(executor)),
        next_callback_(std::move(next_callback)) {
  }

  /// Pull the result handling operator() in
  using proto::result_handler_base<
      HandleResults,
      callback_base<hints::signature_hint_tag<Args...>, HandleResults,
                    HandleErrors, Callback, Executor, NextCallback>,
      hints::signature_hint_tag<Args...>>::operator();

  /// Pull the error handling operator() in
  using proto::error_handler_base<
      HandleErrors,
      callback_base<hints::signature_hint_tag<Args...>, HandleResults,
                    HandleErrors, Callback, Executor, NextCallback>>::
  operator();

  /// Resolves the continuation with the given values
  void set_value(Args... args) {
    std::move (*this)(std::move(args)...);
  }

  /// Resolves the continuation with the given error variable.
  void set_exception(types::error_type error) {
    std::move (*this)(types::dispatch_error_tag{}, std::move(error));
  }
};

template <typename Hint, handle_results HandleResults,
          handle_errors HandleErrors, typename Callback, typename Executor,
          typename NextCallback>
auto make_callback(Callback&& callback, Executor&& executor,
                   NextCallback&& next_callback) {
  return callback_base<Hint, HandleResults, HandleErrors,
                       std::decay_t<Callback>, std::decay_t<Executor>,
                       std::decay_t<NextCallback>>{
      std::forward<Callback>(callback), std::forward<Executor>(executor),
      std::forward<NextCallback>(next_callback)};
}

/// Once this was a workaround for GCC bug:
/// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
struct final_callback {
  template <typename... Args>
  void operator()(Args... /*args*/) && {
  }

  template <typename... Args>
  void set_value(Args... /*args*/) {
  }

  void set_exception(types::error_type error) {
    (void)error;
#ifndef CONTINUABLE_WITH_UNHANDLED_ERRORS
    // There were unhandled errors inside the asynchronous call chain!
    // Define `CONTINUABLE_WITH_UNHANDLED_ERRORS` in order
    // to ignore unhandled errors!"
    util::trap();
#endif // CONTINUABLE_WITH_UNHANDLED_ERRORS
  }
};
} // namespace callbacks

/// Returns the next hint when the callback is invoked with the given hint
template <typename T, typename... Args>
constexpr auto
next_hint_of(std::integral_constant<handle_results, handle_results::yes>,
             traits::identity<T> /*callback*/,
             hints::signature_hint_tag<Args...> /*current*/) {
  // Partial Invoke the given callback
  using Result = decltype(
      util::partial_invoke(std::declval<T>(), std::declval<Args>()...));

  // Return the hint of thr given invoker
  return decltype(decoration::invoker_of(traits::identify<Result>{}).hint()){};
}
/// Don't progress the hint when we don't continue
template <typename T, typename... Args>
constexpr auto
next_hint_of(std::integral_constant<handle_results, handle_results::no>,
             traits::identity<T> /*callback*/,
             hints::signature_hint_tag<Args...> current) {
  return current;
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
template <handle_results HandleResults, handle_errors HandleErrors,
          typename Continuation, typename Callback, typename Executor>
auto chain_continuation(Continuation&& continuation, Callback&& callback,
                        Executor&& executor) {
  static_assert(is_continuation<std::decay_t<Continuation>>{},
                "Expected a continuation!");

  using Hint = decltype(hints::hint_of(traits::identity_of(continuation)));
  constexpr auto next_hint =
      next_hint_of(std::integral_constant<handle_results, HandleResults>{},
                   traits::identify<decltype(callback)>{}, Hint{});

  // TODO consume only the data here so the freeze isn't needed
  auto ownership_ = attorney::ownership_of(continuation);
  continuation.freeze();

  return attorney::create(
      [
        continuation = std::forward<Continuation>(continuation),
        callback = std::forward<Callback>(callback),
        executor = std::forward<Executor>(executor)
      ](auto&& next_callback) mutable {

        // Invokes a continuation with a given callback.
        // Passes the next callback to the resulting continuable or
        // invokes the next callback directly if possible.
        //
        // For example given:
        // - Continuation: continuation<[](auto&& callback) { callback("hi"); }>
        // - Callback: [](std::string) { }
        // - NextCallback: []() { }
        auto proxy =
            callbacks::make_callback<Hint, HandleResults, HandleErrors>(
                std::move(callback), std::move(executor),
                std::forward<decltype(next_callback)>(next_callback));

        // Invoke the continuation with a proxy callback.
        // The proxy callback is responsible for passing
        // the result to the callback as well as decorating it.
        attorney::invoke_continuation(std::move(continuation),
                                      std::move(proxy));
      },
      next_hint, ownership_);
}

/// Final invokes the given continuation chain:
///
/// For example given:
/// - Continuation: continuation<[](auto&& callback) { callback("hi"); }>
template <typename Continuation>
void finalize_continuation(Continuation&& continuation) {
  attorney::invoke_continuation(std::forward<Continuation>(continuation),
                                callbacks::final_callback{});
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
} // namespace base
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_BASE_HPP_INCLUDED
