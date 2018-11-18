
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
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/core/hints.hpp>
#include <continuable/detail/utility/flat-variant.hpp>

namespace cti {
namespace detail {
namespace container {
/// A class similar to the one in the expected proposal,
/// however it is capable of carrying an exception_ptr if
/// exceptions are used.
template <typename T>
class expected {
  flat_variant<T, exception_t> variant_;

public:
  explicit expected() = default;
  explicit expected(expected const&) = default;
  explicit expected(expected&&) = default;
  expected& operator=(expected const&) = default;
  expected& operator=(expected&&) = default;
  ~expected() = default;

  explicit expected(T value) : variant_(std::move(value)) {
  }
  explicit expected(exception_t exception) : variant_(std::move(exception)) {
  }

  expected& operator=(T value) {
    variant_ = std::move(value);
    return *this;
  }
  expected& operator=(exception_t exception) {
    variant_ = std::move(exception);
    return *this;
  }

  void set_value(T value) {
    variant_ = std::move(value);
  }
  void set_exception(exception_t exception) {
    variant_ = std::move(exception);
  }

  bool is_value() const noexcept {
    return variant_.template is<T>();
  }
  bool is_exception() const noexcept {
    return variant_.template is<exception_t>();
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
  exception_t& get_exception() noexcept {
    return variant_.template cast<exception_t>();
  }
  exception_t const& get_exception() const noexcept {
    return variant_.template cast<exception_t>();
  }

  T& operator*() noexcept {
    return get_value();
  }
  T const& operator*() const noexcept {
    return get_value();
  }
};

namespace detail {
struct void_guard_tag {};

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
  using expected_type = expected<std::tuple<First, Second, Rest...>>;

  static auto wrap(First first, Second second, Rest... rest) {
    return std::make_tuple(std::move(first), std::move(second),
                           std::move(rest)...);
  }
  static auto unwrap(expected_type&& e) {
    assert(e.is_value());
    return std::move(e.get_value());
  }
};
} // namespace detail

template <typename Continuable>
using expected_result_trait_t = detail::expected_result_trait<decltype(
    hints::hint_of(traits::identify<Continuable>{}))>;
} // namespace container
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED
