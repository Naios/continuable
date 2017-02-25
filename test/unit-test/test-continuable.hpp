
/**
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

#ifndef TEST_CONTINUABLE_HPP__
#define TEST_CONTINUABLE_HPP__

#include <cassert>

#include "continuable/continuable-base.hpp"
#include "continuable/continuable-testing.hpp"
#include "continuable/continuable.hpp"
#include "gtest/gtest.h"

template <typename Provider>
class continuation_provider : public ::testing::Test, public Provider {
public:
  auto supply() {
    return this->makeVoid([](auto&& callback) mutable {
      // ...
      std::forward<decltype(callback)>(callback)();
    });
  };
  template <typename... Args> auto supply(Args&&... args) {
    return this->template make<std::decay_t<Args>...>([values = std::make_tuple(
                                                           std::forward<Args>(
                                                               args)...)](
        auto&& callback) mutable {
      cti::detail::util::unpack(std::move(values), [&](auto&&... passed) {
        // ...
        std::forward<decltype(callback)>(callback)(
            std::forward<decltype(passed)>(passed)...);
      });
    });
  }
};

inline auto empty_caller() {
  return [](auto&& callback) {
    // ...
    std::forward<decltype(callback)>(callback)();
  };
}

struct provide_copyable {
  template <typename T> auto makeVoid(T&& callback) {
    return make<void>(std::forward<T>(callback));
  }
  template <typename... Args, typename T> auto make(T&& callback) {
    return cti::make_continuable<Args...>(std::forward<T>(callback));
  }
};

struct provide_unique {
  template <typename T> auto makeVoid(T&& callback) {
    return make<void>(std::forward<T>(callback));
  }
  template <typename... Args, typename T> auto make(T&& callback) {
    return cti::make_continuable<Args...>([
      callback = std::forward<T>(callback), guard = std::make_unique<int>(0)
    ](auto&&... args) mutable {
      (void)(*guard);
      return std::move(callback)(std::forward<decltype(args)>(args)...);
    });
  }
};

struct provide_copyable_erasure {
  template <typename T> auto makeVoid(T&& callback) {
    return make(std::forward<T>(callback));
  }
  template <typename... Args, typename T>
  cti::continuable<Args...> make(T&& callback) {
    return cti::make_continuable(std::forward<T>(callback));
  }
};

struct provide_unique_erasure {
  template <typename T> auto makeVoid(T&& callback) {
    return make(std::forward<T>(callback));
  }
  template <typename... Args, typename T>
  cti::unique_continuable<Args...> make(T&& callback) {
    return cti::make_continuable(std::forward<T>(callback));
  }
};

/*
template <typename Left, typename Right> struct provide_continuation_or_left {
  Left left_;
  Right right_;

  template <typename T> auto makeVoid(T&& callback) {
    return left_.template make<void>(std::forward<T>(callback)) ||
           right_.template make<void>(empty_caller());
  }
  template <typename... Args, typename T> auto make(T&& callback) {
    return left_.template make<Args...>(std::forward<T>(callback)) ||
      right_.template make<void>(empty_caller());
  }
};
*/

template <typename... Args> struct type_chainer {
  template <typename First> auto add() {
    return type_chainer<Args..., First>{};
  }
  template <template <typename> class T, typename First> auto add() {
    return type_chainer<Args..., T<First>>{};
  }
  template <template <typename, typename> class T, typename First,
            typename Second>
  auto add() {
    return type_chainer<Args..., T<First, Second>, T<First, Second>,
                        T<Second, First>, T<Second, Second>>{};
  }

  using type = testing::Types<Args...>;
};

inline auto make_type() {
  type_chainer<> chainer{};
  return chainer // ...
      .add<provide_copyable>()
      .add<provide_unique>()
      .add<provide_copyable_erasure>()
      .add<provide_unique_erasure>();
}

using single_types = decltype(make_type())::type;

struct tag1 {};
struct tag2 {};
struct tag3 {};

template <typename Provider>
struct single_dimension_tests : continuation_provider<Provider> {};

TYPED_TEST_CASE(single_dimension_tests, single_types);

#endif // TEST_CONTINUABLE_HPP__
