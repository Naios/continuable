
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

#ifndef CONTINUABLE_TESTING_HPP_INCLUDED
#define CONTINUABLE_TESTING_HPP_INCLUDED

#include <continuable/continuable-api.hpp>
#include <continuable/detail/testing.hpp>
#include <continuable/detail/traits.hpp>

/// Asserts that the final callback of the given continuable was called
/// with any result.
///
/// \since version 1.0.0
#define ASSERT_ASYNC_COMPLETION(CONTINUABLE)                                   \
  cti::detail::testing::assert_async_completion(CONTINUABLE);

/// Asserts that the final callback of the given continuable was called
/// with any exceptional result.
///
/// \since version 2.0.0
#define ASSERT_ASYNC_EXCEPTION_COMPLETION(CONTINUABLE)                         \
  cti::detail::testing::assert_async_exception_completion(CONTINUABLE);

/// Asserts that the final callback of the given continuable is never called
/// with any result.
///
/// \since version 1.0.0
#define ASSERT_ASYNC_INCOMPLETION(CONTINUABLE)                                 \
  cti::detail::testing::assert_async_never_completed(CONTINUABLE);

/// Expects the continuation to be called and forwards it's arguments to
/// the given validator which can then do assertions on the result.
#define ASSERT_ASYNC_VALIDATION(CONTINUABLE, VALIDATOR)                        \
  cti::detail::testing::assert_async_validation(CONTINUABLE, VALIDATOR);

/// Asserts that the continuation was called and forwards it's arguments to
/// the given validator which can then do assertions on the result.
///
/// A validator consists of a binary consumer with a signature as in
/// in the example shown below:
/// ```cpp
/// auto validator = [](auto expected, auto actual) {
///   EXPECT_EQ(expected, actual);
/// };
/// ```
///
/// The macro is usable as shown in the following example:
/// ```cpp
/// continuable<string> async_get(std::string);
/// // ...
/// auto validator = [](auto expected, auto actual) {
///   EXPECT_EQ(expected, actual);
/// };
///
/// ASSERT_ASYNC_BINARY_VALIDATION(validator, async_get("hello"), "hello")
/// ```
///
/// The validator is called for every expecting and actual result.
///
/// \note This macro is mainly present for building other assertions
///       relying on custom validation logic.
///
/// \since version 1.0.0
#define ASSERT_ASYNC_BINARY_VALIDATION(VALIDATOR, ...)                         \
  cti::detail::testing::assert_async_binary_validation(VALIDATOR, __VA_ARGS__);

/// Asserts that the continuation was resolved through an error and forwards
/// it's error result to the given validator which can then do assertions on the
/// error result.
///
/// \note This macro is mainly present for building other assertions
///       relying on custom validation logic.
///
/// \since version 2.0.0
#define ASSERT_ASYNC_BINARY_EXCEPTION_VALIDATION(VALIDATOR, ...)               \
  cti::detail::testing::assert_async_binary_exception_validation(VALIDATOR,    \
                                                                 __VA_ARGS__);

/// Expects that the continuable is finished with the given result
///
/// ```cpp
/// continuable<string> async_get(std::string);
/// // ...
///
/// EXPECT_ASYNC_RESULT(async_get("hello"), "hello");
/// ```
///
/// \since version 1.0.0
#define EXPECT_ASYNC_RESULT(...)                                               \
  ASSERT_ASYNC_BINARY_VALIDATION(cti::detail::testing::expecting_eq_check(),   \
                                 __VA_ARGS__)

/// Asserts that the continuable is finished with the given exception
///
/// \since version 2.0.0
#define EXPECT_ASYNC_EXCEPTION_RESULT(...)                                     \
  ASSERT_ASYNC_BINARY_EXCEPTION_VALIDATION(                                    \
      cti::detail::testing::expecting_eq_check(), __VA_ARGS__)

/// Asserts that the continuable is finished with the given result
///
/// ```cpp
/// continuable<string> async_get(std::string);
/// // ...
///
/// ASSERT_ASYNC_RESULT(async_get("hello"), "hello");
/// ```
///
/// \since version 1.0.0
#define ASSERT_ASYNC_RESULT(...)                                               \
  ASSERT_ASYNC_BINARY_VALIDATION(cti::detail::testing::asserting_eq_check(),   \
                                 __VA_ARGS__)

/// Asserts that the continuable is finished with the given type of arguments
/// without validating it against equality.
///
/// ```cpp
/// continuable<string> async_get(std::string);
/// // ...
///
/// ASSERT_ASYNC_TYPES(async_get("hello"), std::string);
/// ```
///
/// \note This is a compile-time assertion.
///
/// \since version 1.0.0
#define ASSERT_ASYNC_TYPES(CONTINUABLE, ...)                                   \
  cti::detail::testing::assert_async_types(                                    \
      CONTINUABLE, cti::detail::traits::identity<__VA_ARGS__>{})

/// Asserts that the continuable is finished with the given exception
///
/// \since version 2.0.0
#define ASSERT_ASYNC_EXCEPTION_RESULT(...)                                     \
  ASSERT_ASYNC_BINARY_EXCEPTION_VALIDATION(                                    \
      cti::detail::testing::asserting_eq_check(), __VA_ARGS__)

#endif // CONTINUABLE_TESTING_HPP_INCLUDED
