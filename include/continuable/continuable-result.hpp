
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

#ifndef CONTINUABLE_RESULT_HPP_INCLUDED
#define CONTINUABLE_RESULT_HPP_INCLUDED

#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/utility/flat-variant.hpp>
#include <continuable/detail/utility/result-trait.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
/// A class which is convertible to any result and that definitly holds no
/// value so the real result gets invalidated when
/// this object is passed to it
struct empty_result {};

/// A class which is convertible to any result and that definitly holds
/// an exception which is then passed to the converted result object.
class exceptional_result {
  exception_t exception_;

public:
  exceptional_result() = delete;
  exceptional_result(exceptional_result const&) = default;
  exceptional_result(exceptional_result&&) = default;
  exceptional_result& operator=(exceptional_result const&) = default;
  exceptional_result& operator=(exceptional_result&&) = default;
  ~exceptional_result() = default;

  explicit exceptional_result(exception_t exception)
      // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
      : exception_(std::move(exception)) {
  }

  exceptional_result& operator=(exception_t exception) {
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

/// A class similar to the one in the result proposal,
/// however it's capable of carrying an exception_t.
// TODO -> async_result
template <typename... T>
class result {
  using trait = detail::result_trait<T...>;
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
  explicit result(A = {}) {
  }
  explicit result(result const&) = default;
  explicit result(result&&) = default;
  result& operator=(result const&) = default;
  result& operator=(result&&) = default;
  ~result() = default;

  explicit result(T... values) : variant_(trait::wrap(std::move(values)...)) {
  }
  explicit result(exception_t exception) : variant_(std::move(exception)) {
  }
  explicit result(empty_result){};
  explicit result(exceptional_result exceptional_result)
      : variant_(std::move(exceptional_result.get_exception())) {
  }

  result& operator=(empty_result) {
    set_empty();
    return *this;
  }
  result& operator=(exceptional_result exceptional_result) {
    set_exception(std::move(exceptional_result.get_exception()));
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

template <typename T>
struct is_result : std::false_type {};
template <typename... T>
struct is_result<result<T...>> : std::true_type {};
template <>
struct is_result<empty_result> : std::true_type {};
template <>
struct is_result<exceptional_result> : std::true_type {};

template <typename... T>
constexpr auto make_result(T&&... values) {
  return result<detail::traits::unrefcv_t<T>...>(std::forward<T>(values)...);
}

// NOLINTNEXTLINE(performance-unnecessary-value-param)
inline auto make_exceptional_result(exception_t exception) {
  // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
  return exceptional_result(std::move(exception));
}

inline auto make_empty_result() {
  return empty_result{};
}
} // namespace cti

#endif // CONTINUABLE_RESULT_HPP_INCLUDED
