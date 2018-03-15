
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

#ifndef CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED
#define CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <continuable/detail/flat-variant.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace container {
namespace detail {
/// A specialization which returns one of multiple error codes
template <std::size_t I, typename Expected>
struct expected_access;
} // namespace detail

/// A class similar to the one in the expected proposal,
/// however it is capable of carrying an exception_ptr if
/// exceptions are used.
template <typename T>
class expected {
  flat_variant<T, types::error_type> variant_;

public:
  explicit expected() = default;
  explicit expected(expected const&) = default;
  explicit expected(expected&&) = default;
  expected& operator=(expected const&) = default;
  expected& operator=(expected&&) = default;
  ~expected() = default;

  explicit expected(T value) : variant_(std::move(value)) {
  }
  explicit expected(types::error_type exception)
      : variant_(std::move(exception)) {
  }

  expected& operator=(T value) {
    variant_ = std::move(value);
    return *this;
  }
  expected& operator=(types::error_type exception) {
    variant_ = std::move(exception);
    return *this;
  }

  void set_value(T value) {
    variant_ = std::move(value);
  }
  void set_exception(types::error_type exception) {
    variant_ = std::move(exception);
  }

  bool is_value() const noexcept {
    return variant_.template is<T>();
  }
  bool is_exception() const noexcept {
    return variant_.template is<types::error_type>();
  }

  explicit constexpr operator bool() const noexcept {
    return is_value();
  }

  T& get_value() noexcept {
    return variant_.template cast<T>();
  }
  T const& get_value() const noexcept {
    return variant_.template cast<T>();
  }
  types::error_type& get_exception() noexcept {
    return variant_.template cast<types::error_type>();
  }
  types::error_type const& get_exception() const noexcept {
    return variant_.template cast<types::error_type>();
  }

  T& operator*() noexcept {
    return get_value();
  }
  T const& operator*() const noexcept {
    return get_value();
  }

  template <std::size_t I>
  auto get() const& {
    return detail::expected_access<I, expected>::access(*this);
  }
  template <std::size_t I>
  auto get() && {
    return detail::expected_access<I, expected>::access(std::move(*this));
  }
};

namespace detail {
struct void_guard_tag {};

template <typename... Args>
struct multiple_guard_tag {
  std::tuple<Args...> args;
};

template <typename T>
struct expected_result_trait;
template <>
struct expected_result_trait<traits::identity<>> {
  using expected_type = expected<void_guard_tag>;

  static constexpr void_guard_tag wrap() noexcept {
    return {};
  }
  static void unwrap(expected_type&& e) {
    assert(e.is_value());
    (void)e;
  }
};
template <typename T>
struct expected_result_trait<traits::identity<T>> {
  using expected_type = expected<T>;

  static auto wrap(T arg) {
    return std::move(arg);
  }
  static auto unwrap(expected_type&& e) {
    assert(e.is_value());
    return std::move(e.get_value());
  }
};
template <typename First, typename Second, typename... Rest>
struct expected_result_trait<traits::identity<First, Second, Rest...>> {
  using expected_type =
      expected<multiple_guard_tag<std::tuple<First, Second, Rest...>>>;

  static auto wrap(First first, Second second, Rest... rest) {
    using tag = multiple_guard_tag<std::decay_t<First>, std::decay_t<Second>,
                                   std::decay_t<Rest>...>;
    return tag{std::make_tuple(std::move(first), std::move(second),
                               std::move(rest)...)};
  }
  static auto unwrap(expected_type&& e) {
    assert(e.is_value());
    return std::move(e.get_value().args_);
  }
};
} // namespace detail

template <typename Continuable>
using expected_result_trait_t = detail::expected_result_trait<decltype(
    hints::hint_of(traits::identify<Continuable>{}))>;

namespace detail {
/// The specialization which returns an error code instead of a value
template <typename Expected>
struct expected_access<0U, Expected> {
  template <typename T>
  static types::error_type access(expected<T> const& e) {
    if (e.is_exception()) {
      return e.get_exception();
    } else {
      return T{};
    }
  }
  template <typename T>
  static types::error_type access(expected<T>&& e) {
    if (e.is_exception()) {
      return std::move(e.get_exception());
    } else {
      return T{};
    }
  }
};
template <typename T>
struct expected_access<1U, expected<T>> {
  static T access(expected<T> const& e) {
    if (e.is_value()) {
      return e.get_value();
    } else {
      return T{};
    }
  }
  static T access(expected<T>&& e) {
    if (e.is_value()) {
      return std::move(e.get_value());
    } else {
      return T{};
    }
  }
};
template <std::size_t I, typename... Args>
struct expected_access<I, expected<multiple_guard_tag<Args...>>> {
  static auto access(expected<multiple_guard_tag<Args...>> const& e) {
    if (e.is_value()) {
      return std::get<I - 1>(e.get_value()._args);
    } else {
      // Default construct the value
      return std::tuple_element_t<I - 1, std::tuple<Args...>>{};
    }
  }
  static auto access(expected<multiple_guard_tag<Args...>>&& e) {
    if (e.is_value()) {
      return std::get<I - 1>(std::move(e.get_value()._args));
    } else {
      // Default construct the value
      return std::tuple_element_t<I - 1, std::tuple<Args...>>{};
    }
  }
};
} // namespace detail

/// Get specializtion mainly for using for co_await together
/// with structured bindings.
/*template <std::size_t I, typename T>
constexpr auto get(expected<T> const& e) /// ...
    -> decltype(detail::expected_access<I, expected<T>>(e)) {
  return detail::expected_access<I, expected<T>>(e);
}
/// Get specializtion mainly for using for co_await together
/// with structured bindings.
template <std::size_t I, typename T>
constexpr auto get(expected<T>&& e) /// ...
    -> decltype(detail::expected_access<I, expected<T>>(std::move(e))) {
  return detail::expected_access<I, expected<T>>(std::move(e));
}*/
} // namespace container
} // namespace detail
} // namespace cti

/// \cond false
namespace std {
template <typename T>
struct tuple_size<cti::detail::container::expected<T>>
    : integral_constant<std::size_t, 2> {};
template <typename... Args>
struct tuple_size<cti::detail::container::expected<
    cti::detail::container::detail::multiple_guard_tag<Args...>>>
    : integral_constant<std::size_t, 1 + sizeof...(Args)> {};
} // namespace std
/// \endcond

#endif // CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED
