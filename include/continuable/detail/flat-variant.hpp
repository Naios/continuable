
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
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <continuable/detail/traits.hpp>

namespace cti {
namespace detail {
namespace container {
namespace detail {
// We don't want to pull the algorithm header in
template <typename... T>
constexpr std::size_t max_element_of(std::initializer_list<std::size_t> list) {
  std::size_t m = 0;
  for (auto current : list) {
    if (current > m) {
      m = current;
    }
  }
  return m;
}
template <typename... T>
constexpr std::size_t storage_of_impl() {
  constexpr auto size = max_element_of({sizeof(T)...});
  constexpr auto align = max_element_of({alignof(T)...});
  return std::aligned_storage_t<size, align>{};
}

/// Declares the aligned storage union for the given types
template <typename... T>
using storage_of_t = decltype(storage_of_impl<T...>());

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
    assert(!other.is_empty());

#ifndef _NDEBUG
    me.set_slot(empty_slot::value);
#endif

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set_slot(other.get());
    other.destroy();
  }
  flat_variant_move_base& operator=(flat_variant_move_base const&) = default;
  flat_variant_move_base& operator=(flat_variant_move_base&& right) {
    Base& me = *static_cast<Base*>(this);
    Base& other = *static_cast<Base*>(&right);
    assert(!other.is_empty());

    me.weak_destroy();

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set_slot(other.get());
    other.destroy();
    return *this;
  }
};
template <typename Base, bool IsCopyable /*= true*/>
struct flat_variant_copy_base : flat_variant_move_base<Base> {
  constexpr flat_variant_copy_base() = default;

  flat_variant_copy_base(flat_variant_copy_base&&) = default;
  explicit flat_variant_copy_base(flat_variant_copy_base const& right)
      : flat_variant_copy_base<Base>()
  // TODO noexcept(Base::is_nothrow_move_constructible)
  {
    Base& me = *static_cast<Base*>(this);
    Base const& other = *static_cast<Base const*>(&right);
    assert(!other.is_empty());

#ifndef _NDEBUG
    me.set_slot(empty_slot::value);
#endif

    other.visit([&](auto&& value) {
      // ...
      me.init(std::move(value));
    });
    me.set_slot(other.get());
  }
  flat_variant_copy_base& operator=(flat_variant_copy_base&&) = default;
  flat_variant_copy_base& operator=(flat_variant_copy_base const& right)
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
    me.set_slot(other.get());
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
class flat_variant
    : detail::flat_variant_copy_base<
          flat_variant<T...>,
          detail::every<std::is_copy_constructible, T...>::value>,
      detail::flat_variant_base<T...> {

  template <typename...>
  friend class flat_variant;
  template <typename>
  friend struct detail::flat_variant_move_base;
  template <typename, bool>
  friend struct detail::flat_variant_copy_base;

  template <typename V>
  flat_variant(V&& value, detail::slot_t const slot) {
    using type = std::decay_t<V>;
    new (&this->storage_) type(std::forward<V>(value));
    set_slot(slot);
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

  template <typename V, std::size_t Index =
                            traits::index_of_t<std::decay_t<V>, T...>::value>
  // Since the flat_variant is never a part of the contained
  // values itself this overload is safed against the linted issue.
  // NOLINTNEXTLINE(misc-forwarding-reference-overload)
  explicit flat_variant(V&& value)
      : flat_variant(std::forward<V>(value), Index) {
  }

  template <typename V, std::size_t Index =
                            traits::index_of_t<std::decay_t<V>, T...>::value>
  flat_variant& operator=(V&& value) {
    weak_destroy();
    init(std::forward<V>(value));
    set_slot(Index);
    return *this;
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
  V& cast() noexcept {
    assert(is_slot(traits::index_of_t<std::decay_t<V>, T...>::value));
    return *reinterpret_cast<std::decay_t<V>*>(&this->storage_);
  }

  template <typename V>
  V const& cast() const noexcept {
    assert(is_slot(traits::index_of_t<std::decay_t<V>, T...>::value));
    return *reinterpret_cast<std::decay_t<V> const*>(&this->storage_);
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
      constexpr callback_t const callbacks[] = {
          &visit_dispatch<T, V>... // ...
      };
      callbacks[get()](this, std::forward<V>(visitor));
    }
  }
  template <typename V>
  void visit(V&& visitor) const {
    if (!is_empty()) {
      using callback_t = void (*)(flat_variant const*, V&&);
      constexpr callback_t const callbacks[] = {
          &visit_dispatch_const<T, V>... // ...
      };
      callbacks[get()](this, std::forward<V>(visitor));
    }
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
  detail::slot_t get() const noexcept {
    return this->slot_;
  }
  bool is_slot(detail::slot_t const slot) const noexcept {
    return get() == slot;
  }
  void set_slot(detail::slot_t const slot) {
    this->slot_ = slot;
  }
};
} // namespace container
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_FLAT_VARIANT_HPP_INCLUDED
