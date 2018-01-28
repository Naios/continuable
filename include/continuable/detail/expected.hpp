
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

#ifndef CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED__

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

#include <continuable/continuable-api.hpp>
#include <continuable/detail/hints.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>

namespace cti {
namespace detail {
namespace util {
namespace detail {
enum class slot_t { empty, value, error };

template <typename T>
using storage_of_t = //
    std::aligned_storage_t<(sizeof(types::error_type) > sizeof(T)
                                ? sizeof(types::error_type)
                                : sizeof(T))>;

template <typename T>
struct expected_base {
  storage_of_t<T> storage_;
  slot_t slot_;

  constexpr expected_base() : slot_(slot_t::empty) {
  }

  expected_base(expected_base const&) noexcept {
  }
  expected_base(expected_base&&) noexcept {
  }
  expected_base& operator=(expected_base const&) {
    return *this;
  }
  expected_base& operator=(expected_base&&) {
    return *this;
  }
};

template <typename Base>
struct expected_move_base {
  constexpr expected_move_base() = default;

  expected_move_base(expected_move_base const&) = default;
  explicit expected_move_base(expected_move_base&& right) {
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
  expected_move_base& operator=(expected_move_base const&) = default;
  expected_move_base& operator=(expected_move_base&& right) {
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
struct expected_copy_base : expected_move_base<Base> {
  constexpr expected_copy_base() = default;

  expected_copy_base(expected_copy_base&&) = default;
  explicit expected_copy_base(expected_copy_base const& right)
      : expected_move_base<Base>()
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
  expected_copy_base& operator=(expected_copy_base&&) = default;
  expected_copy_base& operator=(expected_copy_base const& right)
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
struct expected_copy_base<Base, false> : expected_move_base<Base> {
  constexpr expected_copy_base() = default;

  expected_copy_base(expected_copy_base const&) = delete;
  explicit expected_copy_base(expected_copy_base&& right) = default;
  expected_copy_base& operator=(expected_copy_base const&) = delete;
  expected_copy_base& operator=(expected_copy_base&& right) = default;
};
} // namespace detail

/// A class similar to the one in the expected proposal,
/// however it is capable of carrying an exception_ptr if
/// exceptions are used.
template <typename T>
class expected
    : detail::expected_copy_base<
          expected<T>, std::is_copy_constructible<types::error_type>::value &&
                           std::is_copy_constructible<T>::value>,
      detail::expected_base<T> {

  template <typename>
  friend class expected;
  template <typename>
  friend struct detail::expected_move_base;
  template <typename, bool>
  friend struct detail::expected_copy_base;

  template <typename V>
  expected(V&& value, detail::slot_t const slot) {
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
    hints::hint_of(traits::identity_of<Continuable>()))>;
} // namespace util
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED__
