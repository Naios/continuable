
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

#ifndef CONTINUABLE_DETAIL_TESTING_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TESTING_HPP_INCLUDED

#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
namespace testing {
template <typename C>
void assert_async_completion(C&& continuable) {
  auto called = std::make_shared<bool>(false);
  std::forward<C>(continuable)
      .then([called](auto&&... args) {
        ASSERT_FALSE(*called);
        *called = true;

        // Workaround for our known GCC bug.
        util::unused(std::forward<decltype(args)>(args)...);
      })
      .fail([](cti::exception_t /*error*/) {
        // ...
        FAIL();
      });

  ASSERT_TRUE(*called);
}

template <typename C>
void assert_async_exception_completion(C&& continuable) {
  auto called = std::make_shared<bool>(false);
  std::forward<C>(continuable)
      .then([](auto&&... args) {
        // Workaround for our known GCC bug.
        util::unused(std::forward<decltype(args)>(args)...);

        // ...
        FAIL();
      })
      .fail([called](cti::exception_t error) {
        ASSERT_TRUE(bool(error));
        ASSERT_FALSE(*called);
        *called = true;
      });

  ASSERT_TRUE(*called);
}

template <typename C>
void assert_async_cancellation(C&& continuable) {
  auto called = std::make_shared<bool>(false);
  std::forward<C>(continuable)
      .then([](auto&&... args) {
        // Workaround for our known GCC bug.
        util::unused(std::forward<decltype(args)>(args)...);

        // ...
        FAIL();
      })
      .fail([called](cti::exception_t error) {
        ASSERT_FALSE(bool(error));
        ASSERT_FALSE(*called);
        *called = true;
      });

  ASSERT_TRUE(*called);
}

template <typename C>
void assert_async_never_completed(C&& continuable) {
  std::forward<C>(continuable)
      .then([](auto&&... args) {
        // Workaround for our known GCC bug.
        util::unused(std::forward<decltype(args)>(args)...);

        FAIL();
      })
      .fail([](cti::exception_t) {
        // ...
        FAIL();
      });
}

template <typename C, typename V>
void assert_async_validation(C&& continuable, V&& validator) {
  assert_async_completion(
      std::forward<C>(continuable)
          .then(
              [validator = std::forward<V>(validator)](auto&&... args) mutable {
                validator(std::forward<decltype(args)>(args)...);
              }));
}

/// Expects that the continuable is finished with the given arguments
template <typename V, typename C, typename... Args>
void assert_async_binary_validation(V&& validator, C&& continuable,
                                    Args&&... expected) {

  using size = std::integral_constant<std::size_t, sizeof...(expected)>;

  assert_async_validation(
      std::forward<C>(continuable),
      [expected_pack = std::make_tuple(std::forward<Args>(expected)...),
       validator = std::forward<V>(validator)](auto&&... args) mutable {
        static_assert(size::value == sizeof...(args),
                      "Async completion handler called with a different count "
                      "of arguments!");

        validator(std::make_tuple(std::forward<decltype(args)>(args)...),
                  expected_pack);
      });
}

/// Expects that the continuable is finished with the given arguments
template <typename V, typename C, typename Args>
void assert_async_binary_exception_validation(V&& validator, C&& continuable,
                                              Args&& expected) {
  auto called = std::make_shared<bool>(false);
  std::forward<C>(continuable)
      .then([](auto&&... args) {
        // Workaround for our known GCC bug.
        util::unused(std::forward<decltype(args)>(args)...);

        // The exception was not thrown!
        FAIL();
      })
      .fail([called, validator = std::forward<decltype(validator)>(validator),
             expected = std::forward<decltype(expected)>(expected)](
                exception_t error) {
        ASSERT_FALSE(*called);
        *called = true;

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
        try {
          std::rethrow_exception(error);
        } catch (std::decay_t<decltype(expected)> const& exception) {
          validator(exception, expected);
        } catch (...) {
          FAIL();
        }
#else
        validator(error, expected);
#endif
      });

  ASSERT_TRUE(*called);
}

inline auto expecting_eq_check() {
  return [](auto&& expected, auto&& actual) {
    EXPECT_EQ(std::forward<decltype(expected)>(expected),
              std::forward<decltype(actual)>(actual));
  };
}

inline auto asserting_eq_check() {
  return [](auto&& expected, auto&& actual) {
    ASSERT_EQ(std::forward<decltype(expected)>(expected),
              std::forward<decltype(actual)>(actual));
  };
}

template <typename... Expected>
struct assert_async_types_validator {
  template <typename... Actual>
  void operator()(Actual...) {
    static_assert(
        std::is_same<identity<Actual...>, identity<Expected...>>::value,
        "The called arguments don't match with the expected ones!");
  }
};

template <typename C, typename... Args>
void assert_async_types(C&& continuable, identity<Args...> /*expected*/) {
  assert_async_validation(std::forward<C>(continuable),
                          assert_async_types_validator<Args...>{});
}
} // namespace testing
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TESTING_HPP_INCLUDED
