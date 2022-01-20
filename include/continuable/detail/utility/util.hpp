
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.2.0

  Copyright(c) 2015 - 2022 Denis Blank <denis.blank at outlook dot com>

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED
#define CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED

#include <cassert>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>

/// Hint for the compiler that this point should be unreachable
#if defined(_MSC_VER)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE_INTRINSIC() __assume(false)
#elif defined(__GNUC__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE_INTRINSIC() __builtin_unreachable()
#elif defined(__has_builtin) && __has_builtin(__builtin_unreachable)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE_INTRINSIC() __builtin_unreachable()
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE_INTRINSIC() abort()
#endif

/// Causes the application to exit abnormally
#if defined(_MSC_VER)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_TRAP() __debugbreak()
#elif defined(__GNUC__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_TRAP() __builtin_trap()
#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_TRAP() __builtin_trap()
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_TRAP() *(volatile int*)0x11 = 0
#endif

#ifndef NDEBUG
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE() ::cti::detail::util::unreachable_debug()
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CTI_DETAIL_UNREACHABLE() CTI_DETAIL_UNREACHABLE_INTRINSIC()
#endif

namespace cti {
namespace detail {
/// Utility namespace which provides useful meta-programming support
namespace util {
#ifndef NDEBUG
  [[noreturn]] inline void unreachable_debug() {
    CTI_DETAIL_TRAP();
    std::abort();
  }
#endif

/// Helper to trick compilers about that a parameter pack is used
template <typename... T>
constexpr void unused(T&&...) noexcept {
}

namespace detail {
template <typename T, std::size_t... I>
auto forward_except_last_impl(T&& tuple,
                              std::integer_sequence<std::size_t, I...>) {
  (void)tuple;
  return std::forward_as_tuple(std::get<I>(std::forward<T>(tuple))...);
}

template <std::size_t Size>
constexpr auto make_decreased_index_sequence(
    std::integral_constant<std::size_t, Size>) noexcept {
  return std::make_index_sequence<Size - 1>();
}
inline void make_decreased_index_sequence(
    std::integral_constant<std::size_t, 0U>) noexcept {
  // This function is only instantiated on a compiler error and
  // should not be included in valid code.
  // See https://github.com/Naios/continuable/issues/21 for details.
  CTI_DETAIL_UNREACHABLE();
}

/// Forwards every element in the tuple except the last one
template <typename T>
auto forward_except_last(T&& sequenceable) {
  static_assert(
      std::tuple_size<std::decay_t<T>>::value > 0U,
      "Attempt to remove a parameter from an empty tuple like type! If you see "
      "this your compiler could run into possible infinite recursion! Open a "
      "ticket at https://github.com/Naios/continuable/issues with a small "
      "reproducible example if your compiler doesn't stop!");

  constexpr auto size = std::tuple_size<std::decay_t<T>>::value;
  constexpr auto sequence = make_decreased_index_sequence(
      std::integral_constant<std::size_t, size>{});

  return forward_except_last_impl(std::forward<T>(sequenceable), sequence);
}

template <std::size_t Keep>
struct invocation_env {
  /// We are able to call the callable with the arguments given in the tuple
  template <typename T, typename... Args>
  static auto partial_invoke_impl(std::true_type, T&& callable,
                                  std::tuple<Args...> args) {
    return traits::unpack(std::forward<T>(callable), std::move(args));
  }

  /// We were unable to call the callable with the arguments in the tuple.
  /// Remove the last argument from the tuple and try it again.
  template <typename T, typename... Args>
  static auto partial_invoke_impl(std::false_type, T&& callable,
                                  std::tuple<Args...> args) {

    // If you are encountering this assertion you tried to attach a callback
    // which can't accept the arguments of the continuation.
    //
    // ```cpp
    // continuable<int, int> c;
    // std::move(c).then([](std::vector<int> v) { /*...*/ })
    // ```
    static_assert(
        sizeof...(Args) > Keep,
        "There is no way to call the given object with these arguments!");

    // Remove the last argument from the tuple
    auto next = forward_except_last(std::move(args));

    // Test whether we are able to call the function with the given tuple
    constexpr std::integral_constant<
        bool, traits::is_invocable_from_tuple<decltype(callable),
                                              decltype(next)>::value ||
                  (sizeof...(Args) <= Keep)>
        is_callable;

    return partial_invoke_impl(is_callable, std::forward<T>(callable),
                               std::move(next));
  }

  /// Shortcut - we can call the callable directly
  template <typename T, typename... Args>
  static auto partial_invoke_impl_shortcut(std::true_type, T&& callable,
                                           Args&&... args) {
    return std::forward<T>(callable)(std::forward<Args>(args)...);
  }

  /// Failed shortcut - we were unable to invoke the callable with the
  /// original arguments.
  template <typename T, typename... Args>
  static auto partial_invoke_impl_shortcut(std::false_type failed, T&& callable,
                                           Args&&... args) {

    // Our shortcut failed, convert the arguments into a forwarding tuple
    return partial_invoke_impl(
        failed, std::forward<T>(callable),
        std::forward_as_tuple(std::forward<Args>(args)...));
  }
};
} // namespace detail

/// Partially invokes the given callable with the given arguments.
///
/// \note This function will assert statically if there is no way to call the
///       given object with less arguments.
template <std::size_t KeepArgs, typename T, typename... Args>
/*keep this inline*/ inline auto
partial_invoke(std::integral_constant<std::size_t, KeepArgs>, T&& callable,
               Args&&... args) {
  // Test whether we are able to call the function with the given arguments.
  constexpr traits::is_invocable_from_tuple<decltype(callable),
                                            std::tuple<Args...>>
      is_invocable;

  // The implementation is done in a shortcut way so there are less
  // type instantiations needed to call the callable with its full signature.
  using env = detail::invocation_env<KeepArgs>;
  return env::partial_invoke_impl_shortcut(
      is_invocable, std::forward<T>(callable), std::forward<Args>(args)...);
}

/// Invokes the given callable object with the given arguments
template <typename Callable, typename... Args>
constexpr auto invoke(Callable&& callable, Args&&... args) noexcept(
    noexcept(std::forward<Callable>(callable)(std::forward<Args>(args)...)))
    -> decltype(std::forward<Callable>(callable)(std::forward<Args>(args)...)) {

  return std::forward<Callable>(callable)(std::forward<Args>(args)...);
}
/// Invokes the given member function pointer by reference
template <typename T, typename Type, typename Self, typename... Args>
constexpr auto invoke(Type T::*member, Self&& self, Args&&... args) noexcept(
    noexcept((std::forward<Self>(self).*member)(std::forward<Args>(args)...)))
    -> decltype((std::forward<Self>(self).*
                 member)(std::forward<Args>(args)...)) {
  return (std::forward<Self>(self).*member)(std::forward<Args>(args)...);
}
/// Invokes the given member function pointer by pointer
template <typename T, typename Type, typename Self, typename... Args>
constexpr auto invoke(Type T::*member, Self&& self, Args&&... args) noexcept(
    noexcept((std::forward<Self>(self)->*member)(std::forward<Args>(args)...)))
    -> decltype(
        (std::forward<Self>(self)->*member)(std::forward<Args>(args)...)) {
  return (std::forward<Self>(self)->*member)(std::forward<Args>(args)...);
}

/// Returns a constant view on the object
template <typename T>
constexpr std::add_const_t<T>& as_const(T& object) noexcept {
  return object;
}

// Class for making child classes non copyable
struct non_copyable {
  constexpr non_copyable() = default;
  non_copyable(non_copyable const&) = delete;
  constexpr non_copyable(non_copyable&&) = default;
  non_copyable& operator=(non_copyable const&) = delete;
  non_copyable& operator=(non_copyable&&) = default;
};

// Class for making child classes non copyable and movable
struct non_movable {
  constexpr non_movable() = default;
  non_movable(non_movable const&) = delete;
  constexpr non_movable(non_movable&&) = delete;
  non_movable& operator=(non_movable const&) = delete;
  non_movable& operator=(non_movable&&) = delete;
};

/// This class is responsible for holding an abstract copy- and
/// move-able ownership that is invalidated when the object
/// is moved to another instance.
class ownership {
  explicit constexpr ownership(bool acquired, bool frozen)
      : acquired_(acquired), frozen_(frozen) {
  }

public:
  constexpr ownership() : acquired_(true), frozen_(false) {
  }
  constexpr ownership(ownership const&) = default;
  ownership(ownership&& right) noexcept
      : acquired_(right.consume()), frozen_(right.is_frozen()) {
  }
  ownership& operator=(ownership const&) = default;
  ownership& operator=(ownership&& right) noexcept {
    acquired_ = right.consume();
    frozen_ = right.is_frozen();
    return *this;
  }

  // Merges both ownerships together
  ownership operator|(ownership const& right) const noexcept {
    return ownership(is_acquired() && right.is_acquired(),
                     is_frozen() || right.is_frozen());
  }

  constexpr bool is_acquired() const noexcept {
    return acquired_;
  }
  constexpr bool is_frozen() const noexcept {
    return frozen_;
  }

  void release() noexcept {
    assert(is_acquired() && "Tried to release the ownership twice!");
    acquired_ = false;
  }
  void freeze(bool enabled = true) noexcept {
    assert(is_acquired() && "Tried to freeze a released object!");
    frozen_ = enabled;
  }

private:
  bool consume() noexcept {
    if (is_acquired()) {
      release();
      return true;
    }
    return false;
  }

  /// Is true when the object is in a valid state
  bool acquired_ : 1;
  /// Is true when the automatic invocation on destruction is disabled
  bool frozen_ : 1;
};
} // namespace util
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_UTIL_HPP_INCLUDED
