
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

#if UNIT_TEST_STEP >= 3
#define THIRD_PARTY_TESTS
#endif

#ifdef THIRD_PARTY_TESTS
// #if _MSC_VER
// #pragma warning(push, 0)
// #endif
#endif // THIRD_PARTY_TESTS

#include <cassert>

#include "continuable/continuable-base.hpp"
#include "continuable/continuable-testing.hpp"
#include "continuable/continuable.hpp"
#include "gtest/gtest.h"
#include <functional>

#ifdef THIRD_PARTY_TESTS

#include "cxx_function/cxx_function.hpp"

template <typename T>
using cxx_function_fn = cxx_function::function<T>;

template <typename... Args>
using cxx_trait_of =
    cti::continuable_trait<cxx_function_fn, cxx_function_fn, Args...>;

template <typename... Args>
using cxx_continuable = typename cxx_trait_of<Args...>::continuable;

template <typename T>
using cxx_function_unique_fn = cxx_function::unique_function<T>;

template <typename... Args>
using unique_cxx_trait_of =
    cti::continuable_trait<cxx_function_unique_fn, cxx_function_unique_fn,
                           Args...>;

template <typename... Args>
using cxx_unique_continuable =
    typename unique_cxx_trait_of<Args...>::continuable;
#endif // THIRD_PARTY_TESTS

template <typename... Args>
using std_trait_of =
    cti::continuable_trait<std::function, std::function, Args...>;

template <typename... Args>
using std_continuable = typename std_trait_of<Args...>::continuable;

using cti::detail::traits::identity;

inline auto to_hint(identity<> /*hint*/) {
  return identity<void>{};
}
template <typename... Args>
auto to_hint(identity<Args...> hint) {
  return hint;
}

template <typename... Args>
auto supplier_of(Args&&... args) {
  return [values = std::make_tuple(std::forward<Args>(args)...)](
      auto&& callback) mutable {
    cti::detail::traits::unpack(std::move(values), [&](auto&&... passed) {
      // ...
      std::forward<decltype(callback)>(callback)(
          std::forward<decltype(passed)>(passed)...);
    });
  };
}

template <typename Provider>
class continuation_provider : public ::testing::Test, public Provider {
public:
  template <typename T>
  auto invoke(T&& type) {
    return this->make(identity<>{}, identity<void>{},
                      [type = std::forward<T>(type)](auto&& callback) mutable {
                        std::forward<decltype(callback)>(callback)();
                      });
  }

  template <typename... Args>
  auto supply(Args&&... args) {
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

template <template <typename...> class Erasure>
struct provide_erasure {
  template <typename... Args, typename... Hint, typename T>
  Erasure<Args...> make(identity<Args...>, identity<Hint...>, T&& callback) {
    return cti::make_continuable<Hint...>(std::forward<T>(callback));
  }
};

template <typename Provider>
struct provide_continuation_and_left {
  Provider provider_;

  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...> args, identity<Hint...> hint, T&& callback) {
    return empty_continuable() &&
           provider_.make(args, hint, std::forward<T>(callback));
  }
};

template <typename Provider>
struct provide_continuation_and_right {
  Provider provider_;

  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...> args, identity<Hint...> hint, T&& callback) {
    return provider_.make(args, hint, std::forward<T>(callback)) &&
           empty_continuable();
  }
};

template <typename Provider>
struct provide_continuation_seq_right {
  Provider provider_;

  template <typename... Args, typename... Hint, typename T>
  auto make(identity<Args...> args, identity<Hint...> hint, T&& callback) {
    return provider_.make(args, hint, std::forward<T>(callback)) >>
           empty_continuable();
  }
};

// clang-format off
// Feel free to uncomment more tests, however this will increase the
// build time significantly.
using single_types = ::testing::Types<
#if UNIT_TEST_STEP == 0
  provide_copyable
  // provide_unique,
  // provide_erasure<cti::continuable>,
  // provide_erasure<cti::unique_continuable>
#elif UNIT_TEST_STEP == 1
  // Some instantiations out commented for compilation speed reasons
  // provide_continuation_and_left<provide_copyable>,
  provide_continuation_and_left<provide_unique>,
  // provide_continuation_and_left<provide_erasure<cti::continuable>>,
  // provide_continuation_and_left<provide_erasure<cti::unique_continuable>>,
  // provide_continuation_and_right<provide_copyable>,
  provide_continuation_and_right<provide_unique>,
  provide_continuation_and_left<provide_erasure<cti::continuable>>
#elif UNIT_TEST_STEP == 2
  provide_continuation_and_left<provide_erasure<cti::unique_continuable>>
#elif UNIT_TEST_STEP == 3
#define NO_ERASURE_TESTS
#define NO_FUTURE_TESTS
  provide_erasure<std_continuable>
#elif UNIT_TEST_STEP == 4
//#define NO_ERASURE_TESTS
//#define NO_FUTURE_TESTS
//  provide_erasure<cxx_continuable>,
//  provide_erasure<cxx_unique_continuable>
  provide_continuation_seq_right<provide_unique>
#endif
>;
// clang-format on

struct tag1 {};
struct tag2 {};
struct tag3 {};

template <typename Provider>
struct single_dimension_tests : continuation_provider<Provider> {};

TYPED_TEST_CASE(single_dimension_tests, single_types);

template <typename T>
auto make_step(T* me, unsigned& current, unsigned step) {
  return me->invoke([=]() mutable {
    ASSERT_EQ(step, current);
    ++current;
  });
}

#endif // TEST_CONTINUABLE_HPP__
