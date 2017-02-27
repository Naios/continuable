
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

using cti::detail::util::identity;

inline auto to_hint(identity<> /*hint*/) { return identity<void>{}; }
template <typename... Args> auto to_hint(identity<Args...> hint) {
  return hint;
}

template <typename... Args> auto supplier_of(Args&&... args) {
  return [values = std::make_tuple(std::forward<Args>(args)...)](
      auto&& callback) mutable {
    cti::detail::util::unpack(std::move(values), [&](auto&&... passed) {
      // ...
      std::forward<decltype(callback)>(callback)(
          std::forward<decltype(passed)>(passed)...);
    });
  };
}

template <typename Provider>
class continuation_provider : public ::testing::Test, public Provider {
public:
  template <typename T> auto invoke(T&& type) {
    return this->make(identity<>{}, identity<void>{},
                      [type = std::forward<T>(type)](auto&& callback) mutable {
                        std::forward<decltype(callback)>(callback)();
                      });
  }

  template <typename... Args> auto supply(Args&&... args) {
    identity<std::decay_t<Args>...> arg_types;
    auto hint_types = to_hint(arg_types);

    return this->make(arg_types, hint_types,
                      supplier_of(std::forward<Args>(args)...));
  }
};

inline auto empty_caller() {
  return [](auto&& callback) {
    // ...
    std::forward<decltype(callback)>(callback)();
  };
}

inline auto empty_continuable() {
  return cti::make_continuable<void>(empty_caller());
}

struct provide_copyable {
  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...>, identity<Hint...>, T&& callback) {
    return cti::make_continuable<Hint...>(std::forward<T>(callback));
  }
};

struct provide_unique {
  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...>, identity<Hint...>, T&& callback) {
    return cti::make_continuable<Hint...>([
      callback = std::forward<T>(callback), guard = std::make_unique<int>(0)
    ](auto&&... args) mutable {
      (void)(*guard);
      return std::move(callback)(std::forward<decltype(args)>(args)...);
    });
  }
};

struct provide_copyable_erasure {
  template <typename... Args, typename... Hint, typename T>
  cti::continuable<Args...> make(identity<Args...>, identity<Hint...>,
                                 T&& callback) {
    return cti::make_continuable<Hint...>(std::forward<T>(callback));
  }
};

struct provide_unique_erasure {
  template <typename... Args, typename... Hint, typename T>
  cti::unique_continuable<Args...> make(identity<Args...>, identity<Hint...>,
                                        T&& callback) {
    return cti::make_continuable<Hint...>(std::forward<T>(callback));
  }
};

template <typename Provider> struct provide_continuation_and_left {
  Provider provider_;

  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...> args, identity<Hint...> hint, T&& callback) {
    return empty_continuable() &&
           provider_.make(args, hint, std::forward<T>(callback));
  }
};

template <typename Provider> struct provide_continuation_and_right {
  Provider provider_;

  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...> args, identity<Hint...> hint, T&& callback) {
    return provider_.make(args, hint, std::forward<T>(callback)) &&
           empty_continuable();
  }
};

// clang-format off
using single_types = ::testing::Types<
  provide_copyable,
  provide_unique,
  provide_copyable_erasure,
  provide_unique_erasure,
  // Some instantiations out commented for compilation speed reasons
  // provide_continuation_and_left<provide_copyable>,
  provide_continuation_and_left<provide_unique>,
// provide_continuation_and_left<provide_copyable_erasure>,
// provide_continuation_and_left<provide_unique_erasure>,
  // Some instantiations out commented for compilation speed reasons
  // provide_continuation_and_right<provide_copyable>,
  provide_continuation_and_right<provide_unique>
  // provide_continuation_and_left<provide_copyable_erasure>,
  // provide_continuation_and_left<provide_unique_erasure>
>;
// clang-format on

struct tag1 {};
struct tag2 {};
struct tag3 {};

template <typename Provider>
struct single_dimension_tests : continuation_provider<Provider> {};

TYPED_TEST_CASE(single_dimension_tests, single_types);

#endif // TEST_CONTINUABLE_HPP__
