
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

#ifndef CONTINUABLE_EXPECTED_HPP_INCLUDED
#define CONTINUABLE_EXPECTED_HPP_INCLUDED

#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/utility/expected-traits.hpp>
#include <continuable/detail/utility/flat-variant.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
/// A class which is convertible to any expected and that definitly holds no
/// value so the real expected gets invalidated when
/// this object is passed to it
struct empty_expected {};

/// A class which is convertible to any expected and that definitly holds
/// an exception which is then passed to the converted expected object.
class exceptional_expected {
  exception_t exception_;

public:
  exceptional_expected() = delete;
  exceptional_expected(exceptional_expected const&) = default;
  exceptional_expected(exceptional_expected&&) = default;
  exceptional_expected& operator=(exceptional_expected const&) = default;
  exceptional_expected& operator=(exceptional_expected&&) = default;
  ~exceptional_expected() = default;

  explicit exceptional_expected(exception_t exception)
      // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
      : exception_(std::move(exception)) {
  }

  exceptional_expected& operator=(exception_t exception) {
    // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
    exception_ = std::move(exception);
    return *this;
  }

  void set_exception(exception_t exception) {
    // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
    exception_ = std::move(exception);
  }

  exception_t& get_exception() & noexcept {
    return exception_;
  }
  exception_t const& get_exception() const& noexcept {
    return exception_;
  }
  exception_t&& get_exception() && noexcept {
    return std::move(exception_);
  }
};

/// A class similar to the one in the expected proposal,
/// however it's capable of carrying an exception_t.
// TODO -> async_result
template <typename... T>
class expected {
  using trait = detail::expected_trait<T...>;
  using value_t = typename trait::value_t;

  detail::container::flat_variant<value_t, exception_t> variant_;

public:
  template <typename A = detail::traits::identity<>,
            // I know this is a little bit hacky but it's a working version
            // of a default constructor that is not present when the class is
            // instantiated with zero arguments.
            std::enable_if_t<((sizeof(A) * 0 + sizeof...(T)) > 0)>* = nullptr,
            std::enable_if_t<
                std::is_same<A, detail::traits::identity<>>::value>* = nullptr>
  explicit expected(A = {}) {
  }
  explicit expected(expected const&) = default;
  explicit expected(expected&&) = default;
  expected& operator=(expected const&) = default;
  expected& operator=(expected&&) = default;
  ~expected() = default;

  explicit expected(T... values) : variant_(trait::wrap(std::move(values)...)) {
  }
  explicit expected(exception_t exception) : variant_(std::move(exception)) {
  }
  explicit expected(empty_expected){};
  explicit expected(exceptional_expected exceptional_expected)
      : variant_(std::move(exceptional_expected.get_exception())) {
  }

  expected& operator=(empty_expected) {
    set_empty();
    return *this;
  }
  expected& operator=(exceptional_expected exceptional_expected) {
    set_exception(std::move(exceptional_expected.get_exception()));
    return *this;
  }

  void set_empty() {
    variant_.set_empty();
  }
  void set_value(T... values) {
    variant_ = trait::wrap(std::move(values)...);
  }
  void set_exception(exception_t exception) {
    variant_ = std::move(exception);
  }

  bool is_empty() const noexcept {
    return variant_.is_empty();
  }
  bool is_value() const noexcept {
    return variant_.template is<value_t>();
  }
  bool is_exception() const noexcept {
    return variant_.template is<exception_t>();
  }

  explicit constexpr operator bool() const noexcept {
    return is_value();
  }

  value_t& get_value() & noexcept {
    return variant_.template cast<value_t>();
  }
  value_t const& get_value() const& noexcept {
    return variant_.template cast<value_t>();
  }
  value_t&& get_value() && noexcept {
    return std::move(variant_).template cast<value_t>();
  }
  exception_t& get_exception() & noexcept {
    return variant_.template cast<exception_t>();
  }
  exception_t const& get_exception() const& noexcept {
    return variant_.template cast<exception_t>();
  }
  exception_t&& get_exception() && noexcept {
    return std::move(variant_).template cast<exception_t>();
  }

  value_t& operator*() & noexcept {
    return get_value();
  }
  value_t const& operator*() const& noexcept {
    return get_value();
  }
  value_t&& operator*() && noexcept {
    return std::move(*this).get_value();
  }
};

template <typename... T>
constexpr auto make_expected(T&&... values) {
  return expected<detail::traits::unrefcv_t<T>...>(std::forward<T>(values));
}

inline auto make_exceptional_expected(exception_t exception) {
  // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
  return exceptional_expected(std::move(exception));
}

inline auto make_empty_expected() {
  return empty_expected{};
}
} // namespace cti

#endif // CONTINUABLE_EXPECTED_HPP_INCLUDED
