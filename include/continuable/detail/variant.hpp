
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

#ifndef CONTINUABLE_DETAIL_VARIANT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_VARIANT_HPP_INCLUDED

#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include <continuable/detail/traits.hpp>

namespace cti {
namespace detail {
namespace util {
namespace detail {
// We don't want to pull the algorithm header in
constexpr std::size_t max_size(std::initializer_list<std::size_t> list) {
  std::size_t m = 0;
  for (auto current : list) {
    if (current > m) {
      m = current;
    }
  }
  return m;
}

template <typename... T>
using storage_of_t =
    std::aligned_storage_t<max_size({sizeof(T)...}), max_size({alignof(T)...})>;

template <typename T>
struct optional_variant_base {
  storage_of_t<T> storage_;
  std::uint8_t slot_;

  constexpr optional_variant_base() : slot_(0U) {
  }

  optional_variant_base(optional_variant_base const&) noexcept {
  }
  optional_variant_base(optional_variant_base&&) noexcept {
  }
  optional_variant_base& operator=(optional_variant_base const&) {
    return *this;
  }
  optional_variant_base& operator=(optional_variant_base&&) {
    return *this;
  }
};

template <typename Base>
struct optional_variant_move_base {
  constexpr optional_variant_move_base() = default;

  optional_variant_move_base(optional_variant_move_base const&) = default;
  explicit optional_variant_move_base(optional_variant_move_base&& right) {
    Base& me = *static_cast<Base*>(this);
    Base& other = *static_cast<Base*>(&right);
    assert(!other.is_empty());

#ifndef _NDEBUG
    me.set(slot_t::empty);
#endif

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set(other.get());
    other.destroy();
  }
  optional_variant_move_base&
  operator=(optional_variant_move_base const&) = default;
  optional_variant_move_base& operator=(optional_variant_move_base&& right) {
    Base& me = *static_cast<Base*>(this);
    Base& other = *static_cast<Base*>(&right);
    assert(!other.is_empty());

    me.weak_destroy();

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set(other.get());
    other.destroy();
    return *this;
  }
};
template <typename Base, bool IsCopyable /*= true*/>
struct optional_variant_copy_base : optional_variant_copy_base<Base> {
  constexpr optional_variant_copy_base() = default;

  optional_variant_copy_base(optional_variant_copy_base&&) = default;
  explicit optional_variant_copy_base(optional_variant_copy_base const& right)
      : optional_variant_copy_base<Base>()
  // TODO noexcept(Base::is_nothrow_move_constructible)
  {
    Base& me = *static_cast<Base*>(this);
    Base const& other = *static_cast<Base const*>(&right);
    assert(!other.is_empty());

#ifndef _NDEBUG
    me.set(slot_t::empty);
#endif

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set(other.get());
  }
  optional_variant_copy_base& operator=(optional_variant_copy_base&&) = default;
  optional_variant_copy_base& operator=(optional_variant_copy_base const& right)
  // TODO  noexcept(Base::is_nothrow_move_constructible)
  {
    Base& me = *static_cast<Base*>(this);
    Base const& other = *static_cast<Base const*>(&right);
    assert(!other.is_empty());

    me.weak_destroy();

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set(other.get());
    return *this;
  }
};
template <typename Base /*, bool IsCopyable = false*/>
struct optional_variant_copy_base<Base, false>
    : optional_variant_move_base<Base> {
  constexpr optional_variant_copy_base() = default;

  optional_variant_copy_base(optional_variant_copy_base const&) = delete;
  explicit optional_variant_copy_base(optional_variant_copy_base&& right) =
      default;
  optional_variant_copy_base&
  operator=(optional_variant_copy_base const&) = delete;
  optional_variant_copy_base&
  operator=(optional_variant_copy_base&& right) = default;
};
} // namespace detail

/// A class similar to the one in the expected proposal,
/// however it is capable of carrying an exception_ptr if
/// exceptions are used.
template <typename... T>
class optional_variant
    : detail::optional_variant_copy_base<
          optional_variant<T...>,
          std::is_copy_constructible<types::error_type>::value &&
              std::is_copy_constructible<T>::value>,
      detail::optional_variant_base<T...> {

  template <typename>
  friend class optional_variant;
  template <typename>
  friend struct detail::optional_variant_move_base;
  template <typename, bool>
  friend struct detail::optional_variant_copy_base;

  template <typename V>
  optional_variant(V&& value, detail::slot_t const slot) {
    using type = std::decay_t<decltype(value)>;
    new (&this->storage_) type(std::forward<V>(value));
    set(slot);
  }

public:
  constexpr expected() = default;
  expected(expected const&) = default;
  expected(expected&&) = default;
  expected& operator=(expected const&) = default;
  expected& operator=(expected&&) = default;

  ~expected() noexcept(
      std::is_nothrow_destructible<T>::value&&
          std::is_nothrow_destructible<types::error_type>::value) {
    weak_destroy();
  }

  explicit expected(T value) //
      : expected(std::move(value), detail::slot_t::value) {
  }
  explicit expected(types::error_type error) //
      : expected(std::move(error), detail::slot_t::error) {
  }

  expected& operator=(T value) {
    set_value(std::move(value));
    return *this;
  }
  expected& operator=(types::error_type error) {
    set_exception(std::move(error));
    return *this;
  }

  bool is_value() const noexcept {
    assert(!is_empty());
    return is(detail::slot_t::value);
  }
  bool is_exception() const noexcept {
    assert(!is_empty());
    return is(detail::slot_t::error);
  }

  explicit constexpr operator bool() const noexcept {
    return is_value();
  }

  void set_value(T value) {
    weak_destroy();
    init(std::move(value));
    set(detail::slot_t::value);
  }
  void set_exception(types::error_type error) {
    weak_destroy();
    init(std::move(error));
    set(detail::slot_t::error);
  }

  T& get_value() noexcept {
    assert(is_value());
    return cast<T>();
  }
  T const& get_value() const noexcept {
    assert(is_value());
    return cast<T>();
  }
  types::error_type& get_exception() noexcept {
    assert(is_exception());
    return cast<types::error_type>();
  }
  types::error_type const& get_exception() const noexcept {
    assert(is_exception());
    return cast<types::error_type>();
  }

  T& operator*() noexcept {
    return get_value();
  }
  T const& operator*() const noexcept {
    return get_value();
  }

private:
  template <typename V>
  void visit(V&& visitor) {
    switch (this->slot_) {
      case detail::slot_t::value:
        return std::forward<V>(visitor)(cast<T>());
      case detail::slot_t::error:
        return std::forward<V>(visitor)(cast<types::error_type>());
      default:
        // We don't visit when there is no value
        break;
    }
  }
  template <typename V>
  void visit(V&& visitor) const {
    switch (this->slot_) {
      case detail::slot_t::value:
        return std::forward<V>(visitor)(cast<T>());
      case detail::slot_t::error:
        return std::forward<V>(visitor)(cast<types::error_type>());
      default:
        // We don't visit when there is no value
        break;
    }
  }

  bool is_empty() const noexcept {
    return is(detail::slot_t::empty);
  }

  template <typename V>
  V& cast() noexcept {
    assert(!is_empty());
    return *reinterpret_cast<V*>(&this->storage_);
  }
  template <typename V>
  V const& cast() const noexcept {
    assert(!is_empty());
    return *reinterpret_cast<V const*>(&this->storage_);
  }

  template <typename V>
  void init(V&& value) {
    assert(is_empty());
    using type = std::decay_t<decltype(value)>;
    new (&this->storage_) type(std::forward<V>(value));
  }
  void destroy() {
    weak_destroy();

#ifdef NDEBUG
    set(detail::slot_t::empty);
#endif
  }
  void weak_destroy() {
    visit([&](auto&& value) {
      using type = std::decay_t<decltype(value)>;
      value.~type();
    });

#ifndef NDEBUG
    set(detail::slot_t::empty);
#endif
  }
  detail::slot_t get() const noexcept {
    return this->slot_;
  }
  bool is(detail::slot_t const slot) const noexcept {
    return get() == slot;
  }
  void set(detail::slot_t const slot) {
    this->slot_ = slot;
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
} // namespace util
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_VARIANT_HPP_INCLUDED
