
/**
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

#ifndef CONTINUABLE_BASE_HPP_INCLUDED__
#define CONTINUABLE_BASE_HPP_INCLUDED__

#include <atomic>
#include <cassert>
#include <future>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cti {
/// \cond false
inline namespace abi_v1 {
/// \endcond

/// A wrapper class to mark a functional class as continuation
/// Such a wrapper class is required to decorate the result of a callback
/// correctly.
template <typename Data, typename Annotation> class continuable_base;

namespace detail {
/// Utility namespace which provides useful meta-programming support
namespace util {

/// \cond false
#define CTI__FOR_EACH_BOOLEAN_BIN_OP(CTI__OP__)                                \
  CTI__OP__(==)                                                                \
  CTI__OP__(!=) CTI__OP__(<=) CTI__OP__(>=) CTI__OP__(<) CTI__OP__(>)
#define CTI__FOR_EACH_BOOLEAN_UNA_OP(CTI__OP__) CTI__OP__(!)
#define CTI__FOR_EACH_INTEGRAL_BIN_OP(CTI__OP__)                               \
  CTI__OP__(*)                                                                 \
  CTI__OP__(/) CTI__OP__(+) CTI__OP__(-) CTI__FOR_EACH_BOOLEAN_BIN_OP(CTI__OP__)
#define CTI__FOR_EACH_INTEGRAL_UNA_OP(CTI__OP__)                               \
  CTI__OP__(~) CTI__FOR_EACH_BOOLEAN_UNA_OP(CTI__OP__)
/// \endcond

template <typename T, T Value>
struct constant : std::integral_constant<T, Value> {
/// \cond false
#define CTI__INST(CTI__OP)                                                     \
  template <typename OT, OT OValue>                                            \
  /*constexpr*/ auto operator CTI__OP(std::integral_constant<OT, OValue>)      \
      const noexcept {                                                         \
    return constant<decltype((Value CTI__OP OValue)),                          \
                    (Value CTI__OP OValue)>{};                                 \
  }
  CTI__FOR_EACH_INTEGRAL_BIN_OP(CTI__INST)
#undef CTI__INST
#define CTI__INST(CTI__OP)                                                     \
  /*constexpr*/ auto operator CTI__OP() const noexcept {                       \
    return constant<decltype((CTI__OP Value)), (CTI__OP Value)>{};             \
  }
  CTI__FOR_EACH_INTEGRAL_UNA_OP(CTI__INST)
#undef CTI__INST
  /// \endcond
};

template <bool Value>
struct constant<bool, Value> : std::integral_constant<bool, Value> {
/// \cond false
#define CTI__INST(CTI__OP)                                                     \
  template <typename OT, OT OValue>                                            \
  /*constexpr*/ auto operator CTI__OP(std::integral_constant<bool, OValue>)    \
      const noexcept {                                                         \
    return constant<bool, (Value CTI__OP OValue)>{};                           \
  }
  CTI__FOR_EACH_BOOLEAN_BIN_OP(CTI__INST)
#undef CTI__INST
#define CTI__INST(CTI__OP)                                                     \
  /*constexpr*/ auto operator CTI__OP() const noexcept {                       \
    return constant<bool, CTI__OP Value>{};                                    \
  }
  CTI__FOR_EACH_BOOLEAN_UNA_OP(CTI__INST)
#undef CTI__INST
  /// \endcond
};

template <bool Value> using bool_constant = constant<bool, Value>;
template <std::size_t Value> using size_constant = constant<std::size_t, Value>;

template <typename T, bool Value>
auto constant_of(std::integral_constant<T, Value> /*value*/ = {}) {
  return constant<T, Value>{};
}
template <std::size_t Value>
auto size_constant_of(
    std::integral_constant<std::size_t, Value> /*value*/ = {}) {
  return size_constant<Value>{};
}
template <bool Value>
auto bool_constant_of(std::integral_constant<bool, Value> /*value*/ = {}) {
  return bool_constant<Value>{};
}

#undef CTI__FOR_EACH_BOOLEAN_BIN_OP
#undef CTI__FOR_EACH_BOOLEAN_UNA_OP
#undef CTI__FOR_EACH_INTEGRAL_BIN_OP
#undef CTI__FOR_EACH_INTEGRAL_UNA_OP

/// Evaluates to the element at position I.
template <std::size_t I, typename... Args>
using at_t = decltype(std::get<I>(std::declval<std::tuple<Args...>>()));

/// Evaluates to an integral constant which represents the size
/// of the given pack.
template <typename... Args> using size_of_t = size_constant<sizeof...(Args)>;

/// A tagging type for wrapping other types
template <typename... T> struct identity {};
template <typename T> struct identity<T> : std::common_type<T> {};

template <typename> struct is_identity : std::false_type {};
template <typename... Args>
struct is_identity<identity<Args...>> : std::true_type {};

template <typename T> identity<std::decay_t<T>> identity_of(T const& /*type*/) {
  return {};
}
template <typename... Args>
identity<Args...> identity_of(identity<Args...> /*type*/) {
  return {};
}
template <typename T> auto identity_of() {
  return std::conditional_t<is_identity<std::decay_t<T>>::value, T,
                            identity<std::decay_t<T>>>{};
}

template <std::size_t I, typename... T> auto get(identity<T...>) {
  return identity_of<at_t<I, T...>>();
}

/// Helper to trick compilers about that a parameter pack is used
template <typename... T> void unused(T&&... args) {
  auto use = [](auto&& type) mutable {
    (void)type;
    return 0;
  };
  auto deduce = {0, use(std::forward<decltype(args)>(args))...};
  (void)deduce;
  (void)use;
}

namespace detail {
// Equivalent to C++17's std::void_t which targets a bug in GCC,
// that prevents correct SFINAE behavior.
// See http://stackoverflow.com/questions/35753920 for details.
template <typename...> struct deduce_to_void : std::common_type<void> {};
} // end namespace detail

/// C++17 like void_t type
template <typename... T>
using void_t = typename detail::deduce_to_void<T...>::type;

namespace detail {
template <typename T, typename Check, typename = void_t<>>
struct is_valid_impl : std::common_type<std::false_type> {};

template <typename T, typename Check>
struct is_valid_impl<T, Check,
                     void_t<decltype(std::declval<Check>()(std::declval<T>()))>>
    : std::common_type<std::true_type> {};

template <typename Type, typename TrueCallback>
void static_if_impl(std::true_type, Type&& type, TrueCallback&& trueCallback) {
  std::forward<TrueCallback>(trueCallback)(std::forward<Type>(type));
}

template <typename Type, typename TrueCallback>
void static_if_impl(std::false_type, Type&& /*type*/,
                    TrueCallback&& /*trueCallback*/) {}

template <typename Type, typename TrueCallback, typename FalseCallback>
auto static_if_impl(std::true_type, Type&& type, TrueCallback&& trueCallback,
                    FalseCallback&& /*falseCallback*/) {
  return std::forward<TrueCallback>(trueCallback)(std::forward<Type>(type));
}

template <typename Type, typename TrueCallback, typename FalseCallback>
auto static_if_impl(std::false_type, Type&& type,
                    TrueCallback&& /*trueCallback*/,
                    FalseCallback&& falseCallback) {
  return std::forward<FalseCallback>(falseCallback)(std::forward<Type>(type));
}
} // end namespace detail

/// Returns the pack size of the given type
template <typename... Args> auto pack_size_of(identity<std::tuple<Args...>>) {
  return size_of_t<Args...>{};
}
/// Returns the pack size of the given type
template <typename First, typename Second>
auto pack_size_of(identity<std::pair<First, Second>>) {
  return size_of_t<First, Second>{};
}
/// Returns the pack size of the given type
template <typename... Args> auto pack_size_of(identity<Args...>) {
  return size_of_t<Args...>{};
}

/// Returns an index sequence of the given type
template <typename T> auto sequenceOf(T&& sequenceable) {
  auto size = pack_size_of(std::forward<T>(sequenceable));
  (void)size;
  return std::make_index_sequence<decltype(size)::value>();
}

/// Returns a check which returns a true type if the current value
/// is below the
template <std::size_t End> auto isLessThen(size_constant<End> end) {
  return [=](auto current) { return end > current; };
}

/// Compile-time check for validating a certain expression
template <typename T, typename Check>
auto is_valid(T&& /*type*/, Check&& /*check*/) {
  return typename detail::is_valid_impl<T, Check>::type{};
}

/// Creates a static functional validator object.
template <typename Check> auto validatorOf(Check&& check) {
  return [check = std::forward<Check>(check)](auto&& matchable) {
    return is_valid(std::forward<decltype(matchable)>(matchable), check);
  };
}

/// Invokes the callback only if the given type matches the check
template <typename Type, typename Check, typename TrueCallback>
void static_if(Type&& type, Check&& check, TrueCallback&& trueCallback) {
  detail::static_if_impl(std::forward<Check>(check)(type),
                         std::forward<Type>(type),
                         std::forward<TrueCallback>(trueCallback));
}

/// Invokes the callback only if the given type matches the check
template <typename Type, typename Check, typename TrueCallback,
          typename FalseCallback>
auto static_if(Type&& type, Check&& check, TrueCallback&& trueCallback,
               FalseCallback&& falseCallback) {
  return detail::static_if_impl(std::forward<Check>(check)(type),
                                std::forward<Type>(type),
                                std::forward<TrueCallback>(trueCallback),
                                std::forward<FalseCallback>(falseCallback));
}

/// A compile-time while loop, which loops as long the value matches
/// the predicate. The handler shall return the next value.
template <typename Value, typename Predicate, typename Handler>
auto static_while(Value&& value, Predicate&& predicate, Handler&& handler) {
  return static_if(std::forward<Value>(value), predicate,
                   [&](auto&& result) mutable {
                     return static_while(
                         handler(std::forward<decltype(result)>(result)),
                         std::forward<Predicate>(predicate),
                         std::forward<Handler>(handler));
                   },
                   [&](auto&& result) mutable {
                     return std::forward<decltype(result)>(result);
                   });
}

/// Returns a validator which checks whether the given sequenceable is empty
inline auto is_empty() {
  return [](auto const& checkable) {
    return pack_size_of(checkable) == size_constant_of<0>();
  };
}

/// Calls the given unpacker with the content of the given sequence
template <typename U, std::size_t... I>
auto unpack(std::integer_sequence<std::size_t, I...>, U&& unpacker) {
  return std::forward<U>(unpacker)(size_constant_of<I>()...);
}

/// Calls the given unpacker with the content of the given sequenceable
template <typename F, typename U, std::size_t... I>
auto unpack(F&& firstSequenceable, U&& unpacker,
            std::integer_sequence<std::size_t, I...>) {
  using std::get;
  (void)firstSequenceable;
  return std::forward<U>(unpacker)(
      get<I>(std::forward<F>(firstSequenceable))...);
}
/// Calls the given unpacker with the content of the given sequenceable
template <typename F, typename S, typename U, std::size_t... IF,
          std::size_t... IS>
auto unpack(F&& firstSequenceable, S&& secondSequenceable, U&& unpacker,
            std::integer_sequence<std::size_t, IF...>,
            std::integer_sequence<std::size_t, IS...>) {
  using std::get;
  (void)firstSequenceable;
  (void)secondSequenceable;
  return std::forward<U>(unpacker)(
      get<IF>(std::forward<F>(firstSequenceable))...,
      get<IS>(std::forward<S>(secondSequenceable))...);
}
/// Calls the given unpacker with the content of the given sequenceable
template <typename F, typename U>
auto unpack(F&& firstSequenceable, U&& unpacker) {
  return unpack(std::forward<F>(firstSequenceable), std::forward<U>(unpacker),
                sequenceOf(identity_of(firstSequenceable)));
}
/// Calls the given unpacker with the content of the given sequenceables
template <typename F, typename S, typename U>
auto unpack(F&& firstSequenceable, S&& secondSequenceable, U&& unpacker) {
  return unpack(std::forward<F>(firstSequenceable),
                std::forward<S>(secondSequenceable), std::forward<U>(unpacker),
                sequenceOf(identity_of(firstSequenceable)),
                sequenceOf(identity_of(secondSequenceable)));
}

/// Applies the handler function to each element contained in the sequenceable
template <typename Sequenceable, typename Handler>
void static_for_each_in(Sequenceable&& sequenceable, Handler&& handler) {
  unpack(
      std::forward<Sequenceable>(sequenceable), [&](auto&&... entries) mutable {
        auto consume = [&](auto&& entry) mutable {
          handler(std::forward<decltype(entry)>(entry));
          return 0;
        };
        // Apply the consume function to every entry inside the pack
        auto deduce = {0, consume(std::forward<decltype(entries)>(entries))...};
        (void)deduce;
        (void)consume;
      });
}

/// Adds the given type at the back of the left sequenceable
template <typename Left, typename Element>
auto push(Left&& left, Element&& element) {
  return unpack(std::forward<Left>(left), [&](auto&&... leftArgs) {
    return std::make_tuple(std::forward<decltype(leftArgs)>(leftArgs)...,
                           std::forward<Element>(element));
  });
}

/// Adds the element to the back of the identity
template <typename... Args, typename Element>
auto push(identity<Args...>, identity<Element>) {
  return identity<Args..., Element>{};
}

/// Removes the first element from the identity
template <typename First, typename... Rest>
auto pop_first(identity<First, Rest...>) {
  return identity<Rest...>{};
}

/// Returns the merged sequence
template <typename Left> auto merge(Left&& left) {
  return std::forward<Left>(left);
}
/// Merges the left sequenceable with the right ones
template <typename Left, typename Right, typename... Rest>
auto merge(Left&& left, Right&& right, Rest&&... rest) {
  // Merge the left with the right sequenceable
  auto merged =
      unpack(std::forward<Left>(left), std::forward<Right>(right),
             [&](auto&&... args) {
               // Maybe use: template <template<typename...> class T,
               // typename... Args>
               return std::make_tuple(std::forward<decltype(args)>(args)...);
             });
  // And merge it with the rest
  return merge(std::move(merged), std::forward<Rest>(rest)...);
}
/// Merges the left identity with the right ones
template <typename... LeftArgs, typename... RightArgs, typename... Rest>
auto merge(identity<LeftArgs...> /*left*/, identity<RightArgs...> /*right*/,
           Rest&&... rest) {
  return merge(identity<LeftArgs..., RightArgs...>{},
               std::forward<Rest>(rest)...);
}

/// Combines the given arguments with the given folding function
template <typename F, typename First> auto fold(F&& /*folder*/, First&& first) {
  return std::forward<First>(first);
}
/// Combines the given arguments with the given folding function
template <typename F, typename First, typename Second, typename... Rest>
auto fold(F&& folder, First&& first, Second&& second, Rest&&... rest) {
  auto res = folder(std::forward<First>(first), std::forward<Second>(second));
  return fold(std::forward<F>(folder), std::move(res),
              std::forward<Rest>(rest)...);
}

/// Returns a folding function using operator `&&`.
inline auto and_folding() {
  return [](auto&& left, auto&& right) {
    return std::forward<decltype(left)>(left) &&
           std::forward<decltype(right)>(right);
  };
}
/// Returns a folding function using operator `||`.
inline auto or_folding() {
  return [](auto&& left, auto&& right) {
    return std::forward<decltype(left)>(left) ||
           std::forward<decltype(right)>(right);
  };
}
/// Returns a folding function using operator `>>`.
inline auto seq_folding() {
  return [](auto&& left, auto&& right) {
    return std::forward<decltype(left)>(left) >>
           std::forward<decltype(right)>(right);
  };
}

/// Deduces to a std::false_type
template <typename T>
using fail = std::integral_constant<bool, !std::is_same<T, T>::value>;

namespace detail {
template <typename T, typename Args, typename = void_t<>>
struct is_invokable_impl : std::common_type<std::false_type> {};

template <typename T, typename... Args>
struct is_invokable_impl<
    T, std::tuple<Args...>,
    void_t<decltype(std::declval<T>()(std::declval<Args>()...))>>
    : std::common_type<std::true_type> {};
} // end namespace detail

/// Deduces to a std::true_type if the given type is callable with the arguments
/// inside the given tuple.
/// The main reason for implementing it with the detection idiom instead of
/// hana like detection is that MSVC has issues with capturing raw template
/// arguments inside lambda closures.
///
/// ```cpp
/// util::is_invokable_t<object, std::tuple<Args...>>
/// ```
template <typename T, typename Args>
using is_invokable_t = typename detail::is_invokable_impl<T, Args>::type;

namespace detail {
/// Forwards every element in the tuple except the last one
template <typename T> auto forward_except_last(T&& sequenceable) {
  auto size = pack_size_of(identity_of(sequenceable)) - size_constant_of<1>();
  auto sequence = std::make_index_sequence<size.value>();

  return unpack(std::forward<T>(sequenceable),
                [](auto&&... args) {
                  return std::forward_as_tuple(
                      std::forward<decltype(args)>(args)...);
                },
                sequence);
}

/// We are able to call the callable with the arguments given in the tuple
template <typename T, typename... Args>
auto partial_invoke_impl(std::true_type, T&& callable,
                         std::tuple<Args...> args) {
  return unpack(std::move(args), [&](auto&&... arg) {
    return std::forward<T>(callable)(std::forward<decltype(arg)>(arg)...);
  });
}

/// We were unable to call the callable with the arguments in the tuple.
/// Remove the last argument from the tuple and try it again.
template <typename T, typename... Args>
auto partial_invoke_impl(std::false_type, T&& callable,
                         std::tuple<Args...> args) {

  // If you are encountering this assertion you tried to attach a callback
  // which can't accept the arguments of the continuation.
  //
  // ```cpp
  // continuable<int, int> c;
  // std::move(c).then([](std::vector<int> v) { /*...*/ })
  // ```
  static_assert(
      sizeof...(Args) > 0,
      "There is no way to call the given object with these arguments!");

  // Remove the last argument from the tuple
  auto next = forward_except_last(std::move(args));

  // Test whether we are able to call the function with the given tuple
  is_invokable_t<decltype(callable), decltype(next)> is_invokable;

  return partial_invoke_impl(is_invokable, std::forward<T>(callable),
                             std::move(next));
}

/// Shortcut - we can call the callable directly
template <typename T, typename... Args>
auto partial_invoke_impl_shortcut(std::true_type, T&& callable,
                                  Args&&... args) {
  return std::forward<T>(callable)(std::forward<Args>(args)...);
}

/// Failed shortcut - we were unable to invoke the callable with the
/// original arguments.
template <typename T, typename... Args>
auto partial_invoke_impl_shortcut(std::false_type failed, T&& callable,
                                  Args&&... args) {

  // Our shortcut failed, convert the arguments into a forwarding tuple
  return partial_invoke_impl(
      failed, std::forward<T>(callable),
      std::forward_as_tuple(std::forward<Args>(args)...));
}
} // end namespace detail

/// Partially invokes the given callable with the given arguments.
///
/// \note This function will assert statically if there is no way to call the
///       given object with less arguments.
template <typename T, typename... Args>
auto partial_invoke(T&& callable, Args&&... args) {
  // Test whether we are able to call the function with the given arguments.
  is_invokable_t<decltype(callable), std::tuple<Args...>> is_invokable;

  // The implementation is done in a shortcut way so there are less
  // type instantiations needed to call the callable with its full signature.
  return detail::partial_invoke_impl_shortcut(
      is_invokable, std::forward<T>(callable), std::forward<Args>(args)...);
}

// Class for making child classes non copyable
struct non_copyable {
  non_copyable() = default;
  non_copyable(non_copyable const&) = delete;
  non_copyable(non_copyable&&) = default;
  non_copyable& operator=(non_copyable const&) = delete;
  non_copyable& operator=(non_copyable&&) = default;
};

// Class for making child classes non copyable and movable
struct non_movable {
  non_movable() = default;
  non_movable(non_movable const&) = delete;
  non_movable(non_movable&&) = delete;
  non_movable& operator=(non_movable const&) = delete;
  non_movable& operator=(non_movable&&) = delete;
};

/// This class is responsible for holding an abstract copy- and
/// move-able ownership that is invalidated when the object
/// is moved to another instance.
class ownership {
public:
  ownership() = default;
  explicit ownership(bool isOwning_) : is_owning(isOwning_) {}
  ownership(ownership const&) = default;
  ownership(ownership&& right) noexcept
      : is_owning(std::exchange(right.is_owning, false)){};
  ownership& operator=(ownership const&) = default;
  ownership& operator=(ownership&& right) noexcept {
    is_owning = std::exchange(right.is_owning, false);
    return *this;
  }

  ownership operator&&(ownership right) const noexcept {
    return ownership(has_ownership() && right.has_ownership());
  }

  bool has_ownership() const noexcept { return is_owning; }
  void invalidate() noexcept { is_owning = false; }

private:
  bool is_owning = true;
};
} // end namespace util

/// Represents a present signature hint
template <typename... Args> using signature_hint_tag = util::identity<Args...>;
/// Represents an absent signature hint
struct absent_signature_hint_tag {};

template <typename> struct is_absent_hint : std::false_type {};
template <>
struct is_absent_hint<absent_signature_hint_tag> : std::true_type {};

struct this_thread_executor_tag {};

/// The namespace `base` provides the low level API for working
/// with continuable types.
///
/// Important methods are:
/// - Creating a continuation from a callback taking functional
///   base::make_continuable_base(auto&& callback)
///     -> base::continuation<auto>
/// - Chaining a continuation together with a callback
///   base::chain_continuation(base::continuation<auto> continuation,
///                            auto&& callback)
///     -> base::continuation<auto>
/// - Finally invoking the continuation chain
///    base::finalize_continuation(base::continuation<auto> continuation)
///     -> void
namespace base {
/// Returns the signature hint of the given continuable
template <typename T> auto hint_of(util::identity<T>) {
  static_assert(util::fail<T>::value,
                "Expected a continuation with an existing signature hint!");
  return util::identity_of<void>();
}
/// Returns the signature hint of the given continuable
template <typename Data, typename... Args>
auto hint_of(
    util::identity<continuable_base<Data, signature_hint_tag<Args...>>>) {
  return signature_hint_tag<Args...>{};
}

/// Makes a continuation wrapper from the given argument
template <typename T, typename A = absent_signature_hint_tag>
auto make_continuable_base(T&& continuation,
                           A /*hint*/ = absent_signature_hint_tag{}) {
  return continuable_base<std::decay_t<T>, std::decay_t<A>>(
      std::forward<T>(continuation));
}

template <typename T> struct is_continuation : std::false_type {};
template <typename Data, typename Annotation>
struct is_continuation<continuable_base<Data, Annotation>> : std::true_type {};

/// Helper class to access private methods and members of
/// the continuable_base class.
struct attorney {
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
  static Data&& consumeData(continuable_base<Data, Annotation>&& continuation) {
    return std::move(continuation).consumeData();
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
template <typename T, typename Hint> class invoker : public T {
public:
  explicit invoker(T invoke) : T(std::move(invoke)) {}

  using T::operator();

  /// Returns the underlaying signature hint
  Hint hint() const noexcept { return {}; }
};

template <typename T, typename... Args>
auto make_invoker(T&& invoke, signature_hint_tag<Args...>) {
  return invoker<std::decay_t<T>, signature_hint_tag<Args...>>(
      std::forward<T>(invoke));
}

/// - continuable<?...> -> result(nextCallback);
template <typename Data, typename Annotation>
auto invokerOf(util::identity<continuable_base<Data, Annotation>>) {
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
template <typename T> auto invokerOf(util::identity<T>) {
  return make_invoker(
      [](auto&& callback, auto&& nextCallback, auto&&... args) {
        auto result = std::forward<decltype(callback)>(callback)(
            std::forward<decltype(args)>(args)...);

        std::forward<decltype(nextCallback)>(nextCallback)(std::move(result));
      },
      util::identity_of<T>());
}

/// - void -> nextCallback()
inline auto invokerOf(util::identity<void>) {
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
inline auto sequencedUnpackInvoker() {
  return [](auto&& callback, auto&& nextCallback, auto&&... args) {
    auto result = std::forward<decltype(callback)>(callback)(
        std::forward<decltype(args)>(args)...);

    util::unpack(std::move(result), [&](auto&&... types) {
      std::forward<decltype(nextCallback)>(nextCallback)(
          std::forward<decltype(types)>(types)...);
    });
  };
}

// - std::pair<?, ?> -> nextCallback(?, ?)
template <typename First, typename Second>
auto invokerOf(util::identity<std::pair<First, Second>>) {
  return make_invoker(sequencedUnpackInvoker(),
                      util::identity<First, Second>{});
}

// - std::tuple<?...>  -> nextCallback(?...)
template <typename... Args>
auto invokerOf(util::identity<std::tuple<Args...>>) {
  return make_invoker(sequencedUnpackInvoker(), util::identity<Args...>{});
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
void invoke_proxy(signature_hint_tag<Args...>, Continuation&& continuation,
                  Callback&& callback, Executor&& executor,
                  NextCallback&& nextCallback) {

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
    auto invoker = decoration::invokerOf(result);

    // Invoke the callback
    packed_dispatch(std::move(executor), std::move(invoker),
                    std::move(callback), std::move(nextCallback),
                    std::move(args)...);
  });
}

/// Returns the next hint when the callback is invoked with the given hint
template <typename T, typename... Args>
auto next_hint_of(util::identity<T> /*callback*/,
                  signature_hint_tag<Args...> /*current*/) {
  auto result =
      util::identity_of<decltype(std::declval<T>()(std::declval<Args>()...))>();

  return decoration::invokerOf(result).hint();
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

  return make_continuable_base(
      [
        continuation = std::forward<Continuation>(continuation),
        partial_callable = std::move(partial_callable),
        executor = std::forward<Executor>(executor)
      ](auto&& nextCallback) mutable {
        invoke_proxy(hint_of(util::identity_of(continuation)),
                     std::move(continuation), std::move(partial_callable),
                     std::move(executor),
                     std::forward<decltype(nextCallback)>(nextCallback));
      },
      next_hint);
}

/// Workaround for GCC bug:
/// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64095
struct empty_callback {
  template <typename... Args> void operator()(Args...) const {}
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
template <typename T> class supplier_callback {
  T data_;

public:
  explicit supplier_callback(T data) : data_(std::move(data)) {}

  template <typename... Args> auto operator()(Args...) {
    return std::move(data_);
  }
};

/// Returns a continuable into a functional object returning the continuable
template <typename Continuation>
auto wrap_continuation(Continuation&& continuation) {
  return supplier_callback<std::decay_t<Continuation>>(
      std::forward<Continuation>(continuation));
}
} // end namespace base

/// The namespace `compose` offers methods to chain continuations together
/// with `all`, `any` or `seq` logic.
namespace compose {
struct strategy_all_tag {};
struct strategy_any_tag {};

template <typename T> struct is_strategy : std::false_type {};
template <> struct is_strategy<strategy_all_tag> : std::true_type {};
template <> struct is_strategy<strategy_any_tag> : std::true_type {};

/// Provides support for extracting the signature hint out
/// of given types and parameters.
namespace annotating {
namespace detail {
/// Void hints are equal to an empty signature
inline auto make_hint_of(util::identity<void>) {
  return signature_hint_tag<>{};
}
/// All other hints are the obvious hints...
template <typename... HintArgs>
auto make_hint_of(util::identity<HintArgs...> args) {
  return args; // Identity is equal to signature_hint_tag
}
} // end namespace detail

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
auto extract(util::identity<T> /*type*/, util::identity<HintArgs...> hint) {
  return util::static_if(hint, util::is_empty(),
                         [=](auto /*hint*/) {
                           /// When the arguments are the hint is absent
                           return absent_signature_hint_tag{};
                         },
                         [](auto hint) {
                           // When hint arguments are given just take it as hint
                           return detail::make_hint_of(hint);
                         });
}
} // end namespace annotating

namespace detail {
template <std::size_t Pos, typename T>
void assign(util::size_constant<Pos> /*pos*/, T& /*storage*/) {
  // ...
}
template <std::size_t Pos, typename T, typename Current, typename... Args>
void assign(util::size_constant<Pos> pos, T& storage, Current&& current,
            Args&&... args) {
  std::get<Pos>(storage) = std::forward<Current>(current);
  assign(pos + util::size_constant_of<1>(), storage,
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
  std::tuple<Args...> result_;

public:
  explicit all_result_submitter(T callback)
      : callback_(std::move(callback)), left_(Submissions) {}

  /// Creates a submitter which submits it's result into the tuple
  template <std::size_t From, std::size_t To>
  auto create_callback(util::size_constant<From> from,
                       util::size_constant<To> to) {

    return [ me = this->shared_from_this(), from, to ](auto&&... args) {
      static_assert(sizeof...(args) == (To - From),
                    "Submission called with the wrong amount of arguments!");

      // Assign the values from the result to it's correct positions of the
      // tuple. Maybe think about the thread safety again...:
      // http://stackoverflow.com/questions/40845699
      assign(from, me->result_, std::forward<decltype(args)>(args)...);

      // Complete the current result
      me->complete_one();
    };
  }

private:
  // Invokes the callback with the cached result
  void invoke() {
    assert((left_ == 0U) && "Expected that the submitter is finished!");
    std::atomic_thread_fence(std::memory_order_acquire);
    util::unpack(std::move(result_), [&](auto&&... args) {
      std::move(callback_)(std::forward<decltype(args)>(args)...);
    });
  }
  // Completes one result
  void complete_one() {
    assert((left_ > 0U) && "Expected that the submitter isn't finished!");

    auto current = --left_;
    if (!current) {
      invoke();
    }
  }
};

/// Invokes the callback with the first arriving result
template <typename T>
class any_result_submitter
    : public std::enable_shared_from_this<any_result_submitter<T>>,
      public util::non_movable {

  T callback_;
  std::once_flag flag_;

public:
  explicit any_result_submitter(T callback) : callback_(std::move(callback)) {}

  /// Creates a submitter which submits it's result to the callback
  auto create_callback() {
    return [me = this->shared_from_this()](auto&&... args) {
      me->invoke(std::forward<decltype(args)>(args)...);
    };
  }

private:
  // Invokes the callback with the given arguments
  template <typename... Args> void invoke(Args&&... args) {
    std::call_once(flag_, std::move(callback_), std::forward<Args>(args)...);
  }
};
} // end namespace detail

/// Adds the given continuation tuple to the left composition
template <typename... LeftArgs, typename... RightArgs>
auto chain_composition(std::tuple<LeftArgs...> leftPack,
                       std::tuple<RightArgs...> rightPack) {

  return util::merge(std::move(leftPack), std::move(rightPack));
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
  return base::attorney::consumeData(std::move(continuation));
}

/// Entry function for connecting two continuables with a given strategy.
template <typename Strategy, typename LData, typename LAnnotation,
          typename RData, typename RAnnotation>
auto connect(Strategy strategy, continuable_base<LData, LAnnotation>&& left,
             continuable_base<RData, RAnnotation>&& right) {

  // Make the new data which consists of a tuple containing
  // all connected continuables.
  auto data = chain_composition(normalize(strategy, std::move(left)),
                                normalize(strategy, std::move(right)));

  // Return a new continuable containing the tuple and holding
  // the current strategy as annotation.
  return base::make_continuable_base(std::move(data), strategy);
}

/// Creates a submitter which caches the intermediate results of `all` chains
template <typename Callback, std::size_t Submissions, typename... Args>
auto make_all_result_submitter(Callback&& callback,
                               util::size_constant<Submissions>,
                               util::identity<Args...>) {
  return std::make_shared<detail::all_result_submitter<
      std::decay_t<decltype(callback)>, Submissions, Args...>>(
      std::forward<decltype(callback)>(callback));
}

/// Finalizes the all logic of a given composition
template <typename Composition>
auto finalize_composition(strategy_all_tag, Composition&& composition) {

  // Merge all signature hints together
  auto signature = util::unpack(composition, [](auto&... entries) {
    return util::merge(base::hint_of(util::identity_of(entries))...);
  });

  return base::make_continuable_base(
      [ signature, composition = std::forward<Composition>(composition) ](
          auto&& callback) mutable {
        // We mark the current 2-dimensional position through a pair:
        // std::pair<size_constant<?>, size_constant<?>>
        //           ~~~~~~~~~~~~~~~~  ~~~~~~~~~~~~~~~~
        //           Continuation pos     Result pos
        auto begin = std::make_pair(util::size_constant_of<0>(),
                                    util::size_constant_of<0>());
        auto pack = util::identity_of<std::decay_t<Composition>>();
        auto end = util::pack_size_of(pack);
        auto condition = [=](auto pos) { return pos.first < end; };

        // Create the result submitter which caches all results and invokes
        // the final callback upon completion.
        auto submitter = make_all_result_submitter(
            std::forward<decltype(callback)>(callback), end, signature);

        // Invoke every continuation with it's callback of the submitter
        util::static_while(begin, condition, [&](auto current) mutable {
          auto entry =
              std::move(std::get<decltype(current.first)::value>(composition));

          // This is the length of the arguments of the current continuable
          auto arg_size =
              util::pack_size_of(base::hint_of(util::identity_of(entry)));

          // The next position in the result tuple
          auto next = current.second + arg_size;

          // Invoke the continuation with the associated submission callback
          base::attorney::invoke_continuation(
              std::move(entry),
              submitter->create_callback(current.second, next));

          return std::make_pair(current.first + util::size_constant_of<1>(),
                                next);
        });
      },
      signature);
}

/// Creates a submitter that continues `any` chains
template <typename Callback>
auto make_any_result_submitter(Callback&& callback) {
  return std::make_shared<
      detail::any_result_submitter<std::decay_t<decltype(callback)>>>(
      std::forward<decltype(callback)>(callback));
}

template <typename T, typename... Args>
T first_of(util::identity<T, Args...>) noexcept;

template <typename Signature, typename... Args>
auto common_result_of(Signature signature, signature_hint_tag<>,
                      Args... /*args*/) {
  /// Assert that the other signatures are empty too which means all signatures
  /// had the same size.
  util::static_for_each_in(util::identity<Args...>{}, [&](auto rest) {
    auto is_empty = (util::pack_size_of(rest) == util::size_constant_of<0>());
    static_assert(is_empty.value, "Expected all continuations to have the same"
                                  "count of arguments!");
  });
  return signature;
}

/// Determine the common result between all continuation which are chained
/// with an `any` strategy, consider two continuations:
/// c1 with `void(int)` and c22 with `void(float)`, the common result shared
/// between both continuations is `void(int)`.
template <typename Signature, typename First, typename... Args>
auto common_result_of(Signature signature, First first, Args... args) {
  auto common =
      util::identity<std::common_type_t<decltype(first_of(first)),
                                        decltype(first_of(args))...>>{};

  return common_result_of(util::push(signature, common), util::pop_first(first),
                          util::pop_first(args)...);
}

/// Finalizes the any logic of a given composition
template <typename Composition>
auto finalize_composition(strategy_any_tag, Composition&& composition) {

  // Determine the shared result between all continuations
  auto signature = util::unpack(composition, [](auto const&... args) {
    return common_result_of(signature_hint_tag<>{},
                            base::hint_of(util::identity_of(args))...);
  });

  return base::make_continuable_base(
      [composition =
           std::forward<Composition>(composition)](auto&& callback) mutable {

        // Create the submitter which calls the given callback once at the first
        // callback invocation.
        auto submitter = make_any_result_submitter(
            std::forward<decltype(callback)>(callback));

        util::static_for_each_in(std::move(composition),
                                 [&](auto&& entry) mutable {
                                   // Invoke the continuation with a submission
                                   // callback
                                   base::attorney::invoke_continuation(
                                       std::forward<decltype(entry)>(entry),
                                       submitter->create_callback());
                                 });
      },
      signature);
}

/// Connects the left and the right continuable to a sequence
///
/// \note This is implemented in an eager way because we would not gain
///       any profit from chaining sequences lazily.
template <typename Left, typename Right>
auto sequential_connect(Left&& left, Right&& right) {
  return std::forward<Left>(left).then([right = std::forward<Right>(right)](
      auto&&... args) mutable {
    return std::move(right).then([previous = std::make_tuple(
                                      std::forward<decltype(args)>(args)...)](
        auto&&... args) mutable {
      return util::merge(
          std::move(previous),
          std::make_tuple(std::forward<decltype(args)>(args)...));
    });
  });
}
} // end namespace compose

/// Provides helper functions to transform continuations to other types
namespace transforms {
/// Provides helper functions and typedefs for converting callback arguments
/// to their types a promise can accept.
template <typename... Args> struct future_trait {
  /// The promise type used to create the future
  using promise_t = std::promise<std::tuple<Args...>>;
  /// Boxes the argument pack into a tuple
  static void resolve(promise_t& promise, Args... args) {
    promise.set_value(std::make_tuple(std::move(args)...));
  }
};
template <> struct future_trait<> {
  /// The promise type used to create the future
  using promise_t = std::promise<void>;
  /// Boxes the argument pack into void
  static void resolve(promise_t& promise) { promise.set_value(); }
};
template <typename First> struct future_trait<First> {
  /// The promise type used to create the future
  using promise_t = std::promise<First>;
  /// Boxes the argument pack into nothing
  static void resolve(promise_t& promise, First first) {
    promise.set_value(std::move(first));
  }
};

template <typename Hint> class promise_callback;

template <typename... Args>
class promise_callback<signature_hint_tag<Args...>>
    : public future_trait<Args...> {

  typename future_trait<Args...>::promise_t promise_;

public:
  promise_callback() = default;
  promise_callback(promise_callback const&) = delete;
  promise_callback(promise_callback&&) = default;
  promise_callback& operator=(promise_callback const&) = delete;
  promise_callback& operator=(promise_callback&&) = delete;

  /// Resolves the promise
  void operator()(Args... args) { this->resolve(promise_, std::move(args)...); }
  /// Returns the future from the promise
  auto get_future() { return promise_.get_future(); }
};

/// Transforms the continuation to a future
template <typename Data, typename Annotation>
auto as_future(continuable_base<Data, Annotation>&& continuable) {
  // Create the promise which is able to supply the current arguments
  auto hint = base::hint_of(util::identity_of(continuable));

  promise_callback<std::decay_t<decltype(hint)>> callback;
  (void)hint;

  // Receive the future
  auto future = callback.get_future();

  // Dispatch the continuation with the promise resolving callback
  std::move(continuable).then(std::move(callback)).done();

  return future;
}
} // end namespace transforms
} // end namespace detail

template <typename Data, typename Annotation> class continuable_base {
  template <typename, typename> friend class continuable_base;
  friend struct detail::base::attorney;

  // The continuation type or intermediate result
  Data data_;
  detail::util::ownership ownership_;

public:
  explicit continuable_base(Data data) : data_(std::move(data)) {}

  /// Constructor taking the data
  template <typename OData, std::enable_if_t<std::is_convertible<
                                std::decay_t<OData>, Data>::value>* = nullptr>
  continuable_base(OData&& data) : data_(std::forward<OData>(data)) {}

  /// Constructor taking the data of other continuables while erasing the hint
  template <typename OData, typename OAnnotation>
  continuable_base(continuable_base<OData, OAnnotation>&& other)
      : continuable_base(std::move(other).materialize().consumeData()) {}

  /// \cond false
  continuable_base(continuable_base&&) = default;
  continuable_base(continuable_base const&) = default;

  continuable_base& operator=(continuable_base&&) = default;
  continuable_base& operator=(continuable_base const&) = default;
  /// \endcond

  /// The destructor automatically invokes the continuable_base
  /// if it wasn't consumed yet.
  ///
  /// In order to invoke the continuable early you may call the
  /// continuable_base::done() method.
  ///
  /// You may release the continuable_base through calling the corresponding
  /// continuable_base::release() method which prevents
  /// the invocation on destruction.
  ~continuable_base() {
    if (ownership_.has_ownership()) {
      std::move(*this).done();
    }
    assert(!ownership_.has_ownership() && "Ownership should be released!");
  }

  template <typename OData, typename OAnnotation>
  auto then(continuable_base<OData, OAnnotation>&& continuation) && {
    return std::move(*this).then(
        detail::base::wrap_continuation(std::move(continuation).materialize()));
  }

  template <typename T, typename E = detail::this_thread_executor_tag>
  auto then(T&& callback,
            E&& executor = detail::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation(std::move(*this).materialize(),
                                            std::forward<T>(callback),
                                            std::forward<E>(executor));
  }

  template <typename OData, typename OAnnotation>
  auto operator&&(continuable_base<OData, OAnnotation>&& right) && {
    right.assert_owning();
    return detail::compose::connect(detail::compose::strategy_all_tag{},
                                    std::move(*this), std::move(right));
  }

  template <typename OData, typename OAnnotation>
  auto operator||(continuable_base<OData, OAnnotation>&& right) && {
    right.assert_owning();
    return detail::compose::connect(detail::compose::strategy_any_tag{},
                                    std::move(*this), std::move(right));
  }

  template <typename OData, typename OAnnotation>
  auto operator>>(continuable_base<OData, OAnnotation>&& right) && {
    right.assert_owning();
    return detail::compose::sequential_connect(std::move(*this),
                                               std::move(right));
  }

  auto futurize() && {
    return detail::transforms::as_future(std::move(*this).materialize());
  }

  void done() && {
    assert(ownership_.has_ownership() &&
           "Tried to finalize a continuable with an invalid state!");
    detail::base::finalize_continuation(std::move(*this));
    assert(!ownership_.has_ownership());
  }

  /// Prevents the automatic invocation on destruction as explained
  /// in continuable_base::~continuable_base().
  void release() noexcept { ownership_.invalidate(); }

private:
  auto materialize() &&
      noexcept(std::is_nothrow_move_constructible<Data>::value) {
    return materializeImpl(std::move(*this));
  }

  template <typename OData, typename OAnnotation,
            std::enable_if_t<
                !detail::compose::is_strategy<OAnnotation>::value>* = nullptr>
  static auto
  materializeImpl(continuable_base<OData, OAnnotation>&& continuable) {
    return std::move(continuable);
  }
  template <typename OData, typename OAnnotation,
            std::enable_if_t<
                detail::compose::is_strategy<OAnnotation>::value>* = nullptr>
  static auto
  materializeImpl(continuable_base<OData, OAnnotation>&& continuable) {
    return detail::compose::finalize_composition(
        OAnnotation{}, std::move(continuable).consumeData());
  }

  Data&& consumeData() && {
    release();
    return std::move(data_);
  }

  void assert_owning() const {
    assert(ownership_.has_ownership() &&
           "Tried to use a released continuable!");
  }
};

/// Creates a continuable_base from a callback taking function.
///
/// \tparam Args The types (signature hint) the given callback is called with.
/// * **Some arguments** indicate the types the callback will be invoked with.
/// ```cpp
/// auto ct = cti::make_continuable<int, std::string>([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(200, "<html>...</html>");
/// });
/// ```
/// * **void as argument** indicates that the callback will be invoked
///   with no arguments:
/// ```cpp
/// auto ct = cti::make_continuable<void>([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)();
/// });
/// ```
/// * **No arguments** indicate that the types are unknown.
///   You should always give the type hint a callback is called with because
///   it's required for intermediate actions like connecting continuables.
///   You may omit the signature hint if you are erasing the type of
///   the continuable right after creation.
/// ```cpp
/// // Never do this:
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(0.f, 'c');
/// });
///
/// // However, you may do this:
/// continuable<float, char> ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(0.f, 'c');
/// });
/// ```
///
/// \param continuation The continuation the continuable is created from.
/// The continuation must be a functional type accepting a callback parameter
/// which represents the object invokable with the asynchronous result of this
/// continuable.
/// ```cpp
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)("result");
/// });
/// ```
/// The callback may be stored or moved.
/// In some cases the callback may be copied if supported by the underlying
/// callback chain, in order to invoke the call chain multiple times.
/// It's recommended to accept any callback instead of erasing it.
/// ```cpp
/// // Good practice:
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)("result");
/// });
///
/// // Good practice using a functional object:
/// struct Continuation {
///   template<typename T>
///   void operator() (T&& continuation) const {
///     // ...
///   }
/// }
///
/// auto ct = cti::make_continuable(Continuation{});
///
/// // Bad practice (because of unnecessary type erasure):
/// auto ct = cti::make_continuable(
///   [](std::function<void(std::string)> callback) {
///     callback("result");
///   });
/// ```
///
/// \returns A continuable_base with unknown template parameters which
///          wraps the given continuation.
///          In order to convert the continuable_base to a known type
///          you need to apply type erasure.
///
/// \note You should always turn the callback into a r-value if possible
///       (`std::move` or `std::forward`) for qualifier correct invokation.
///
/// \since version 1.0.0
template <typename... Args, typename Continuation>
auto make_continuable(Continuation&& continuation) {
  auto hint = detail::compose::annotating::extract(
      detail::util::identity_of(continuation),
      detail::util::identity<Args...>{});

  return detail::base::make_continuable_base(
      std::forward<Continuation>(continuation), hint);
}

/// Connects the given continuables with an *all* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator && for details.
///
/// \since version 1.0.0
template <typename... Continuables>
auto all_of(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return detail::util::fold(detail::util::and_folding(),
                            std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with an *any* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator|| for details.
///
/// \since version 1.0.0
template <typename... Continuables>
auto any_of(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return detail::util::fold(detail::util::or_folding(),
                            std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with a *seq* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator>> for details.
///
/// \since version 1.0.0
template <typename... Continuables>
auto seq_of(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return detail::util::fold(detail::util::seq_folding(),
                            std::forward<Continuables>(continuables)...);
}

template <template <typename> class CallbackWrapper,
          template <typename> class ContinuationWrapper, typename... Args>
using continuable_erasure_of_t =
    CallbackWrapper<void(ContinuationWrapper<void(Args...)>)>;

template <typename Erasure, typename... Args>
using continuable_of_t =
    continuable_base<Erasure, detail::signature_hint_tag<Args...>>;

/*
 * cti::none
 * cti::spreading
 * cti::partialization
 * cti::prepend
 * cti::append
 * cti::through
 */

/// \cond false
} // end inline namespace abi_...
/// \endcond
} // end namespace cti

#endif // CONTINUABLE_BASE_HPP_INCLUDED__
