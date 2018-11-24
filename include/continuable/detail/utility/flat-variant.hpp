
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

#ifndef CONTINUABLE_DETAIL_FLAT_VARIANT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_FLAT_VARIANT_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace container {
namespace detail {
// We don't want to pull the algorithm header in
constexpr std::size_t max_element_of(std::initializer_list<std::size_t> list) {
  std::size_t m = 0;
  for (auto current : list) {
    if (current > m) {
      m = current;
    }
  }
  return m;
}

/// Workarround for a regression introduced in ~ MSVC 15.8.1
template <typename T>
using size_of_helper = std::integral_constant<std::size_t, sizeof(T)>;
template <typename T>
using align_of_helper = std::integral_constant<std::size_t, alignof(T)>;

template <typename... T>
constexpr auto storage_of_impl(traits::identity<T...>) {
  constexpr auto size = max_element_of({(size_of_helper<T>::value)...});
  constexpr auto align = max_element_of({(align_of_helper<T>::value)...});
  return std::aligned_storage_t<size, align>{};
}

/// Declares the aligned storage union for the given types
template <typename... T>
using storage_of_t = decltype(storage_of_impl(traits::identity<T...>{}));

/// The value fpr the empty slot
using slot_t = std::uint8_t;

/// The value which is used to mark the empty slot
using empty_slot =
    std::integral_constant<slot_t, std::numeric_limits<slot_t>::max()>;

template <typename... T>
struct flat_variant_base {
  storage_of_t<T...> storage_;
  slot_t slot_;

  constexpr flat_variant_base() : slot_(empty_slot::value) {
  }

  flat_variant_base(flat_variant_base const&) noexcept {
  }
  flat_variant_base(flat_variant_base&&) noexcept {
  }
  flat_variant_base& operator=(flat_variant_base const&) {
    return *this;
  }
  flat_variant_base& operator=(flat_variant_base&&) {
    return *this;
  }
};

template <typename Base>
struct flat_variant_move_base {
  constexpr flat_variant_move_base() = default;

  flat_variant_move_base(flat_variant_move_base const&) = default;
  explicit flat_variant_move_base(flat_variant_move_base&& right) {
    Base& me = *static_cast<Base*>(this);
    Base& other = *static_cast<Base*>(&right);

    if (other.is_empty()) {
      me.set_slot(empty_slot::value);
    } else {

      other.visit([&](auto&& value) {
#ifndef NDEBUG
        me.set_slot(empty_slot::value);
#endif
        // NOLINTNEXTLINE(misc-move-forwarding-reference)
        me.init(std::move(value), other.get_slot());
      });
    }

    other.destroy();
  }
  flat_variant_move_base& operator=(flat_variant_move_base const&) = default;
  flat_variant_move_base& operator=(flat_variant_move_base&& right) {
    Base& me = *static_cast<Base*>(this);
    Base& other = *static_cast<Base*>(&right);

    me.weak_destroy();

    if (other.is_empty()) {
      me.set_slot(empty_slot::value);
    } else {
      other.visit([&](auto&& value) {
        // ...
        me.init(std::move(value), other.get_slot());
      });
    }
    other.destroy();
    return *this;
  }
};
template <typename Base, bool IsCopyable /*= true*/>
struct flat_variant_copy_base : flat_variant_move_base<Base> {
  constexpr flat_variant_copy_base() = default;

  flat_variant_copy_base(flat_variant_copy_base&&) = default;
  explicit flat_variant_copy_base(flat_variant_copy_base const& right)
      : flat_variant_move_base<Base>()
  // TODO noexcept(Base::is_nothrow_move_constructible)
  {
    Base& me = *static_cast<Base*>(this);
    Base const& other = *static_cast<Base const*>(&right);

    if (other.is_empty()) {
      me.set_slot(empty_slot::value);
    } else {
      other.visit([&](auto&& value) {
#ifndef NDEBUG
        me.set_slot(empty_slot::value);
#endif
        me.init(std::move(value), other.get_slot());
      });
    }
  }
  flat_variant_copy_base& operator=(flat_variant_copy_base&&) = default;
  flat_variant_copy_base& operator=(flat_variant_copy_base const& right)
  // TODO  noexcept(Base::is_nothrow_move_constructible)
  {
    Base& me = *static_cast<Base*>(this);
    Base const& other = *static_cast<Base const*>(&right);

    me.weak_destroy();

    if (other.is_empty()) {
      me.set_slot(empty_slot::value);
    } else {
      other.visit([&](auto&& value) {
        // ...
        me.init(std::move(value), other.get_slot());
      });
    }
    return *this;
  }
};
template <typename Base /*, bool IsCopyable = false*/>
struct flat_variant_copy_base<Base, false> : flat_variant_move_base<Base> {
  constexpr flat_variant_copy_base() = default;

  flat_variant_copy_base(flat_variant_copy_base const&) = delete;
  explicit flat_variant_copy_base(flat_variant_copy_base&& right) = default;
  flat_variant_copy_base& operator=(flat_variant_copy_base const&) = delete;
  flat_variant_copy_base& operator=(flat_variant_copy_base&&) = default;
};

/// Deduces to a true_type if all parameters T satisfy the predicate.
template <template <typename> class Predicate, typename... T>
using every = traits::conjunction<Predicate<T>...>;
} // namespace detail

/// A class similar to the one in the variant proposal,
/// however it is capable of carrying an empty state by default.
template <typename... T>
class flat_variant;

template <typename T>
struct is_flat_variant : std::false_type {};
template <typename... T>
struct is_flat_variant<flat_variant<T...>> : std::true_type {};

template <typename... T>
class flat_variant
    : detail::flat_variant_copy_base<
          flat_variant<T...>,
          detail::every<std::is_copy_constructible, T...>::value>,
      detail::flat_variant_base<T...> {

  static_assert(sizeof...(T) > 0, "At least one paremeter T is required!");

  template <typename...>
  friend class flat_variant;
  template <typename>
  friend struct detail::flat_variant_move_base;
  template <typename, bool>
  friend struct detail::flat_variant_copy_base;

  template <typename V>
  flat_variant(V&& value, detail::slot_t const slot) {
#ifndef NDEBUG
    set_slot(detail::empty_slot::value);
#endif
    init(std::forward<V>(value), slot);
  }

public:
  constexpr flat_variant() = default;
  flat_variant(flat_variant const&) = default;
  flat_variant(flat_variant&&) = default;
  flat_variant& operator=(flat_variant const&) = default;
  flat_variant& operator=(flat_variant&&) = default;

  ~flat_variant() noexcept(
      detail::every<std::is_nothrow_destructible, T...>::value) {
    weak_destroy();
  }

  template <
      typename V,
      std::enable_if_t<!is_flat_variant<std::decay_t<V>>::value>* = nullptr>
  // Since the flat_variant isn't allowed through SFINAE
  // this overload is safed against the linted issue.
  // NOLINTNEXTLINE(misc-forwarding-reference-overload)
  explicit flat_variant(V&& value)
      : flat_variant(std::forward<V>(value),
                     traits::index_of_t<std::decay_t<V>, T...>::value) {
  }

  template <
      typename V,
      std::enable_if_t<!is_flat_variant<std::decay_t<V>>::value>* = nullptr>
  flat_variant& operator=(V&& value) {
    weak_destroy();
    init(std::forward<V>(value),
         traits::index_of_t<std::decay_t<V>, T...>::value);
    return *this;
  }

  void set_empty() {
    weak_destroy();
    set_slot(detail::empty_slot::value);
  }

  template <typename V, std::size_t Index =
                            traits::index_of_t<std::decay_t<V>, T...>::value>
  bool is() const noexcept {
    return is_slot(Index);
  }

  bool is_empty() const noexcept {
    return is_slot(detail::empty_slot::value);
  }

  explicit constexpr operator bool() const noexcept {
    return !is_empty();
  }

  template <typename V>
      V& cast() & noexcept {
    assert(is_slot(traits::index_of_t<std::decay_t<V>, T...>::value));
    return *reinterpret_cast<std::decay_t<V>*>(&this->storage_);
  }

  template <typename V>
  V const& cast() const& noexcept {
    assert(is_slot(traits::index_of_t<std::decay_t<V>, T...>::value));
    return *reinterpret_cast<std::decay_t<V> const*>(&this->storage_);
  }

  template <typename V>
      V&& cast() && noexcept {
    assert(is_slot(traits::index_of_t<std::decay_t<V>, T...>::value));
    auto& value = *reinterpret_cast<std::decay_t<V> const*>(&this->storage_);
    return std::move(value);
  }

private:
  template <typename C, typename V>
  static void visit_dispatch(flat_variant* me, V&& visitor) {
    std::forward<V>(visitor)(me->cast<C>());
  }
  template <typename C, typename V>
  static void visit_dispatch_const(flat_variant const* me, V&& visitor) {
    std::forward<V>(visitor)(me->cast<C>());
  }

  template <typename V>
  void visit(V&& visitor) {
    if (!is_empty()) {
      using callback_t = void (*)(flat_variant*, V &&);
      constexpr callback_t const callbacks[] = {&visit_dispatch<T, V>...};
      callbacks[get_slot()](this, std::forward<V>(visitor));
    }
  }
  template <typename V>
  void visit(V&& visitor) const {
    if (!is_empty()) {
      using callback_t = void (*)(flat_variant const*, V&&);
      constexpr callback_t const callbacks[] = {&visit_dispatch_const<T, V>...};
      callbacks[get_slot()](this, std::forward<V>(visitor));
    }
  }

  template <typename V>
  void init(V&& value, detail::slot_t const slot) {
    assert(is_empty());
    assert(sizeof(this->storage_) >= sizeof(std::decay_t<V>));

    using type = std::decay_t<V>;
    new (&this->storage_) type(std::forward<V>(value));
    set_slot(slot);
  }
  void destroy() {
    weak_destroy();

#ifdef NDEBUG
    set_slot(detail::empty_slot::value);
#endif
  }
  void weak_destroy() {
    visit([&](auto&& value) {
      using type = std::decay_t<decltype(value)>;
      value.~type();
    });

#ifndef NDEBUG
    set_slot(detail::empty_slot::value);
#endif
  }
  detail::slot_t get_slot() const noexcept {
    return this->slot_;
  }
  bool is_slot(detail::slot_t const slot) const noexcept {
    return get_slot() == slot;
  }
  void set_slot(detail::slot_t const slot) {
    this->slot_ = slot;
  }
};
} // namespace container
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_FLAT_VARIANT_HPP_INCLUDED
