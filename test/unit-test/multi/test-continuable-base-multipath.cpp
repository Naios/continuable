
/*
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

#include <test-continuable.hpp>

using namespace cti;

static int const CANARY = 382947;

TYPED_TEST(single_dimension_tests, multipath_result_is_forwardable) {
  EXPECT_ASYNC_RESULT(this->supply().then([](auto&&... canaries) -> result<> {
    //
    return recover(std::forward<decltype(canaries)>(canaries)...);
  }));

  EXPECT_ASYNC_RESULT(
      this->supply(CANARY).then([](auto&&... canaries) -> result<int> {
        //
        return recover(std::forward<decltype(canaries)>(canaries)...);
      }),
      CANARY);

  EXPECT_ASYNC_RESULT(
      this->supply(1, CANARY, 3)
          .then([](auto&&... canaries) -> result<int, int, int> {
            //
            return recover(std::forward<decltype(canaries)>(canaries)...);
          }),
      1, CANARY, 3);
}

TYPED_TEST(single_dimension_tests, multipath_result_is_throwable) {
  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply().then([]() -> exceptional_result {
        //
        return rethrow(supply_test_exception());
      }));

  ASSERT_ASYNC_EXCEPTION_COMPLETION(this->supply().then([]() -> result<> {
    //
    return rethrow(supply_test_exception());
  }));
}

TYPED_TEST(single_dimension_tests, multipath_result_is_cancelable) {
  ASSERT_ASYNC_CANCELLATION(this->supply().then([]() -> cancellation_result {
    //
    return cancel();
  }));

  ASSERT_ASYNC_CANCELLATION(this->supply().then([]() -> result<> {
    //
    return cancel();
  }));
}

TYPED_TEST(single_dimension_tests, multipath_result_is_stoppable) {
  ASSERT_ASYNC_INCOMPLETION(this->supply().then([]() -> empty_result {
    //
    return stop();
  }));

  ASSERT_ASYNC_INCOMPLETION(this->supply().then([]() -> result<> {
    //
    return stop();
  }));
}

TYPED_TEST(single_dimension_tests, multipath_exception_is_recoverable) {
  EXPECT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<>{})
          .fail([](exception_t) -> result<> {
            //
            return recover();
          }));

  EXPECT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t) -> result<int> {
            //
            return recover(CANARY);
          }),
      CANARY);

  EXPECT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<int, int, int>{})
          .fail([](exception_t) -> result<int, int, int> {
            //
            return recover(1, CANARY, 3);
          }),
      1, CANARY, 3);
}

TYPED_TEST(single_dimension_tests, multipath_exception_is_forwardable) {
  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t exception) -> exceptional_result {
            //
            return rethrow(exception);
          }));

  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t exception) -> result<int> {
            //
            return rethrow(exception);
          }));
}

TYPED_TEST(single_dimension_tests, multipath_exception_is_stoppable) {
  ASSERT_ASYNC_INCOMPLETION(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t) -> empty_result {
            //
            return stop();
          }));

  ASSERT_ASYNC_INCOMPLETION(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t) -> result<int> {
            //
            return stop();
          }));
}

TYPED_TEST(single_dimension_tests, multipath_exception_is_continuable) {
  ASSERT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<>{})
          .fail([&](exception_t) {
            //
            return this->supply();
          }));

  ASSERT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([&](exception_t) {
            //
            return this->supply(CANARY);
          }),
      CANARY);

  ASSERT_ASYNC_RESULT(
      this->supply_exception(supply_test_exception(), identity<int, int, int>{})
          .fail([&](exception_t) {
            //
            return this->supply(1, CANARY, 2);
          }),
      1, CANARY, 2);
}

TYPED_TEST(single_dimension_tests, multipath_exception_is_autocanceled) {
  bool caught = false;
  ASSERT_ASYNC_COMPLETION(
      this->supply_exception(supply_test_exception()).fail([&](exception_t) {
        EXPECT_FALSE(caught);
        caught = true;
      }).fail([](exception_t){
        FAIL();
      }));
  ASSERT_TRUE(caught);
}

#if !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
// Enable this test only if we support exceptions
TYPED_TEST(single_dimension_tests, multipath_exception_can_rethrow) {

  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception()).fail([](exception_t) {
        // Throw an exception from inside the exception handler
        throw test_exception();
      }));

  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception())
          .fail([](exception_t) -> empty_result {
            // Throw an exception from inside the exception handler
            throw test_exception();
          }));

  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception())
          .fail([](exception_t) -> exceptional_result {
            // Throw an exception from inside the exception handler
            throw test_exception();
          }));

  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception(), identity<int>{})
          .fail([](exception_t) -> result<int> {
            // Throw an exception from inside the exception handler
            throw test_exception();
          }));
}
#endif
