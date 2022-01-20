
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

#ifndef CONTINUABLE_DETAIL_RESULT_VARIANT_HPP_INCLUDED
#define CONTINUABLE_DETAIL_RESULT_VARIANT_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace container {
enum class result_slot_t : std::uint8_t {
  slot_empty,
  slot_value,
  slot_exception,
};
} // namespace container

struct init_empty_arg_t {};
struct init_result_arg_t {};
struct init_exception_arg_t {};

template <typename T>
class result_variant {
  static constexpr bool is_nothrow_destructible = //
      std::is_nothrow_destructible<T>::value &&
      std::is_nothrow_destructible<exception_t>::value;
  static constexpr bool is_nothrow_move_constructible = //
      std::is_nothrow_move_constructible<T>::value &&
      std::is_nothrow_move_constructible<exception_t>::value;

public:
  result_variant() = default;
  ~result_variant() noexcept(is_nothrow_destructible) {
    destroy();
  }

  explicit result_variant(init_empty_arg_t) noexcept
    : slot_(container::result_slot_t::slot_empty) {}
  explicit result_variant(init_result_arg_t, T value) noexcept(
      std::is_nothrow_destructible<T>::value&&
          std::is_nothrow_move_constructible<T>::value)
    : slot_(container::result_slot_t::slot_value) {
    new (value_ptr()) T(std::move(value));
  }
  explicit result_variant(init_exception_arg_t, exception_t exception) noexcept(
      std::is_nothrow_destructible<exception_t>::value&&
          std::is_nothrow_move_constructible<exception_t>::value)
    : slot_(container::result_slot_t::slot_exception) {
    new (exception_ptr()) exception_t(std::move(exception));
  }

  result_variant(result_variant const&) = delete;
  result_variant& operator=(result_variant const&) = delete;

  result_variant(result_variant&& other) noexcept(
      is_nothrow_destructible&& is_nothrow_move_constructible)
    : slot_(other.slot_) {

    switch (other.slot_) {
      case container::result_slot_t::slot_value: {
        new (value_ptr()) T(std::move(*other.value_ptr()));
        break;
      }
      case container::result_slot_t::slot_exception: {
        new (exception_ptr()) exception_t(std::move(*other.exception_ptr()));
        break;
      }
      default: {
        break;
      }
    }

    other.destroy();
    other.slot_ = container::result_slot_t::slot_empty;
  }

  result_variant& operator=(result_variant&& other) noexcept(
      is_nothrow_destructible&& is_nothrow_move_constructible) {

    destroy();
    slot_ = other.slot_;

    switch (other.slot_) {
      case container::result_slot_t::slot_value: {
        new (value_ptr()) T(std::move(*other.value_ptr()));
        break;
      }
      case container::result_slot_t::slot_exception: {
        new (exception_ptr()) exception_t(std::move(*other.exception_ptr()));
        break;
      }
      default: {
        break;
      }
    }

    other.destroy();
    other.slot_ = container::result_slot_t::slot_empty;
    return *this;
  }

  void set_empty() {
    destroy();
    slot_ = container::result_slot_t::slot_empty;
  }
  void set_value(T value) {
    destroy();
    new (value_ptr()) T(std::move(value));
    slot_ = container::result_slot_t::slot_value;
  }
  void set_exception(exception_t exception) {
    destroy();
    new (exception_ptr()) exception_t(std::move(exception));
    slot_ = container::result_slot_t::slot_exception;
  }

  container::result_slot_t slot() const noexcept {
    return slot_;
  }

  bool is_empty() const noexcept {
    return slot_ == container::result_slot_t::slot_empty;
  }
  bool is_value() const noexcept {
    return slot_ == container::result_slot_t::slot_value;
  }
  bool is_exception() const noexcept {
    return slot_ == container::result_slot_t::slot_exception;
  }

  T& get_value() noexcept {
    assert(is_value());
    return *reinterpret_cast<T*>(&storage_);
  }
  T const& get_value() const noexcept {
    assert(is_value());
    return *reinterpret_cast<T const*>(&storage_);
  }

  exception_t& get_exception() noexcept {
    assert(is_exception());
    return *reinterpret_cast<exception_t*>(&storage_);
  }
  exception_t const& get_exception() const noexcept {
    assert(is_exception());
    return *reinterpret_cast<exception_t const*>(&storage_);
  }

private:
  constexpr T* value_ptr() noexcept {
    return reinterpret_cast<T*>(&storage_);
  }
  constexpr exception_t* exception_ptr() noexcept {
    return reinterpret_cast<exception_t*>(&storage_);
  }

  void destroy() noexcept(is_nothrow_destructible) {
    switch (slot_) {
      case container::result_slot_t::slot_value: {
        value_ptr()->~T();
        break;
      }
      case container::result_slot_t::slot_exception: {
        exception_ptr()->~exception_t();
        break;
      }
      default: {
        break;
      }
    }
  }

  container::result_slot_t slot_{container::result_slot_t::slot_empty};
  std::aligned_storage_t<
      (sizeof(T) > sizeof(exception_t) ? sizeof(T) : sizeof(exception_t)),
      (alignof(T) > alignof(exception_t) ? alignof(T) : alignof(exception_t))>
      storage_;
};
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_RESULT_VARIANT_HPP_INCLUDED
