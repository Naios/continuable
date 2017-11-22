
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2017 Denis Blank <denis.blank at outlook dot com>

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

#include <algorithm>
#include <memory>

#include <continuable/continuable-api.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
namespace detail {
namespace expected {
namespace detail {
enum class slot_t { empty, value, error };

template <typename Base>
struct expected_base_util {
  slot_t slot_;

protected:
  template <typename T>
  void init(T&& value, slot_t slot) {
    set(slot);
    using type = std::decay_t<decltype(value)>;
    auto storage = &base()->storage_;
    new (storage) type(std::forward<decltype(value)>(value));
  }
  void destroy() {
    weak_destroy();
    set(slot_t::empty);
  }
  void weak_destroy() {
    base()->visit([&](auto&& value) {
      using type = std::decay_t<decltype(value)>;
      value.~type();
    });
  }
  slot_t get() const noexcept {
    return slot_;
  }
  bool is(slot_t slot) const noexcept {
    return get() == slot;
  }
  void set(slot_t slot) {
    slot_ = slot;
  }
  slot_t consume(slot_t slot) {
    auto current = get();
    destroy();
    return current;
  }
  Base* base() noexcept {
    return static_cast<Base*>(this);
  }
  Base const* base() const noexcept {
    return static_cast<Base*>(this);
  }
};

template <bool IsCopyable, typename Base>
struct expected_base : expected_base_util<Base> {
  explicit expected_base(expected_base const& right) {
    right.visit([&](auto&& value) {
      this->init(std::forward<decltype(value)>(value));
    });
    set(right.consume());
  }
  expected_base& operator=(expected_base const& right) {
    this->weak_destroy();
    right.visit([&](auto&& value) {
      this->init(std::forward<decltype(value)>(value));
    });
    set(right.consume());
  }
};
} // namespace detail

/// A class similar to the one in the expected proposal,
/// however it is capable of carrying an exception_ptr if
/// exceptions are used.
template <typename T, typename Storage = std::aligned_storage_t<std::max(
                          sizeof(types::error_type), sizeof(T))>>
class expected {
  friend class expected_base;

  Storage storage_;

public:
  explicit expected() : slot_(slot_t::empty) {
  }

  explicit expected(T value) : slot_(slot_t::value) {
  }

  explicit expected(types::error_type error) : slot_(slot_t::value) {
  }

  bool is_empty() const noexcept {
    return slot_ == slot_t::empty;
  }
  bool is_value() const noexcept {
    return slot_ == slot_t::value;
  }
  bool is_error() const noexcept {
    return slot_ == slot_t::error;
  }

  template <typename V>
  void visit(V&& visitor) {
    switch (slot_) {
      case slot_t::value:
        return std::forward<V>(visitor)(static_cast<T*>(&storage_));
      case slot_t::error:
        return std::forward<V>(visitor)(
            static_cast<types::error_type*>(&storage_));
    }

    util::unreachable();
  }
  template <typename V>
  void visit(V&& visitor) const {
    switch (slot_) {
      case slot_t::value:
        return std::forward<V>(visitor)(static_cast<T*>(&storage_));
      case slot_t::error:
        return std::forward<V>(visitor)(
            static_cast<types::error_type*>(&storage_));
    }

    util::unreachable();
  }
};
} // namespace expected
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_EXPECTED_HPP_INCLUDED__
