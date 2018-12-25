
/*
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

#ifndef TEST_CONTINUABLE_HPP__
#define TEST_CONTINUABLE_HPP__

#include <cassert>

#include <gtest/gtest.h>

#include <functional>

#include <continuable/continuable-base.hpp>
#include <continuable/continuable-testing.hpp>
#include <continuable/continuable.hpp>

using cti::detail::identity;
using cti::detail::util::unused;

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
      auto&& promise) mutable {
    cti::detail::traits::unpack(
        [&](auto&&... passed) {
          promise.set_value(std::forward<decltype(passed)>(passed)...);
        },
        std::move(values));
  };
}

template <typename Arg>
auto exception_supplier_of(Arg&& arg) {
  return [arg = std::forward<Arg>(arg)](auto&& promise) mutable {
    promise.set_exception(std::move(arg));
  };
}

template <typename Provider>
class continuation_provider : public ::testing::Test, public Provider {
public:
  template <typename T>
  auto invoke(T&& type) {
    return this->make(identity<>{}, identity<void>{},
                      [type = std::forward<T>(type)](auto&& promise) mutable {
                        promise.set_value();
                      });
  }

  template <typename... Args>
  auto supply(Args&&... args) {
#ifdef UNIT_TEST_READY_CONTINUABLES
    return cti::make_ready_continuable(std::forward<Args>(args)...);
#else
    identity<std::decay_t<Args>...> arg_types;
    auto hint_types = to_hint(arg_types);

    return this->make(arg_types, hint_types,
                      supplier_of(std::forward<Args>(args)...));
#endif // UNIT_TEST_READY_CONTINUABLES
  }

  template <typename Arg, typename Hint = identity<>>
  auto supply_exception(Arg&& arg, Hint hint = {}) {
    return this->make(hint, to_hint(hint),
                      exception_supplier_of(std::forward<Arg>(arg)));
  }
};

inline auto empty_caller() {
  return [](auto&& promise) { promise.set_value(); };
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
using single_types_id = identity<
#if UNIT_TEST_STEP == 0
  provide_copyable
  // provide_erasure<cti::continuable>,
  // provide_erasure<cti::continuable>
#elif UNIT_TEST_STEP == 1
  // Some instantiations out commented for compilation speed reasons
  // provide_continuation_and_left<provide_copyable>,
  provide_continuation_and_left<provide_unique>
  // provide_continuation_and_left<provide_erasure<cti::continuable>>,
  // provide_continuation_and_left<provide_erasure<cti::continuable>>,
  // provide_continuation_and_right<provide_copyable>,
  // provide_continuation_and_right<provide_unique>,
  // provide_continuation_and_left<provide_erasure<cti::continuable>>
#elif UNIT_TEST_STEP == 2
  provide_continuation_and_right<provide_unique>
#elif UNIT_TEST_STEP == 3
  provide_unique
#elif UNIT_TEST_STEP == 4
  //  provide_erasure<cxx_continuable>,
  //  provide_erasure<cxx_unique_continuable>
  provide_continuation_seq_right<provide_unique>
#endif
>;
// clang-format on

template <typename T>
struct to_types;
template <typename... T>
struct to_types<identity<T...>> : std::common_type<::testing::Types<T...>> {};

using single_types = to_types<single_types_id>::type;

struct tag1 {};
struct tag2 {};
struct tag3 {};

template <typename Provider>
struct single_dimension_tests : continuation_provider<Provider> {};

TYPED_TEST_CASE(single_dimension_tests, single_types);

template <typename T, typename First, typename Second>
struct combine_to_type;
template <typename... T, typename First, typename Second>
struct combine_to_type<identity<T...>, First, Second>
    : std::common_type<testing::Types<identity<T, First>..., // ...
                                      identity<T, Second>...>> {};

struct all_connector {
  template <typename Left, typename Right>
  auto op(Left&& left, Right&& right) const {
    return std::forward<Left>(left) && std::forward<Right>(right);
  }

  template <typename... Args>
  auto ag(Args&&... args) const {
    return cti::when_all(std::forward<Args>(args)...);
  }
};
struct seq_connector {
  template <typename Left, typename Right>
  auto op(Left&& left, Right&& right) const {
    return std::forward<Left>(left) >> std::forward<Right>(right);
  }

  template <typename... Args>
  auto ag(Args&&... args) const {
    return cti::when_seq(std::forward<Args>(args)...);
  }
};

using aggregate_types =
    combine_to_type<single_types_id, all_connector, seq_connector>::type;

template <typename Identity>
struct single_aggregate_tests;
template <typename Provider, typename Connector>
struct single_aggregate_tests<identity<Provider, Connector>>
    : continuation_provider<Provider>, Connector {};

TYPED_TEST_CASE(single_aggregate_tests, aggregate_types);

template <typename T>
auto make_step(T* me, unsigned& current, unsigned step) {
  return me->invoke([=]() mutable {
    ASSERT_EQ(step, current);
    ++current;
  });
}

#if !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
struct test_exception : std::exception {
  explicit test_exception() = default;

  bool operator==(test_exception const&) const noexcept {
    return true;
  }
};

test_exception get_test_exception_proto();

inline std::exception_ptr supply_test_exception() {
  try {
    throw get_test_exception_proto();
  } catch (...) {
    return std::current_exception();
  }
}
#else
struct my_error_category : std::error_category {
  const char* name() const noexcept override {
    return "generic name";
  }

  std::string message(int) const override {
    return "generic";
  }
};

std::error_condition get_test_exception_proto();

inline std::error_condition supply_test_exception() {
  return get_test_exception_proto();
}
#endif

#endif // TEST_CONTINUABLE_HPP__
