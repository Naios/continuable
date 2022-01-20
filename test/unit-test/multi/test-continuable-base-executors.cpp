
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
using namespace cti::detail;

TYPED_TEST(single_dimension_tests, are_executor_dispatchable) {
  bool invoked = false;
  auto executor = [&](auto&& work) {
    // We can move the worker object
    auto local = std::forward<decltype(work)>(work);
    ASSERT_FALSE(invoked);
    // We can invoke the worker object
    std::move(local)();
    ASSERT_TRUE(invoked);
  };

  auto chain = this->supply().then(
      [&] {
        ASSERT_FALSE(invoked);
        invoked = true;
      },
      executor);

  ASSERT_ASYNC_COMPLETION(std::move(chain));
}

TYPED_TEST(single_dimension_tests, are_executor_dispatchable_via) {
  bool invoked = false;
  auto executor = [&](auto&& work) {
    // We can move the worker object
    auto local = std::forward<decltype(work)>(work);
    ASSERT_FALSE(invoked);
    // We can invoke the worker object
    std::move(local)();
  };

  auto chain = this->supply().via(executor).then([&] {
    ASSERT_FALSE(invoked);
    invoked = true;
  });

  ASSERT_ASYNC_COMPLETION(std::move(chain));

  ASSERT_TRUE(invoked);
}

TYPED_TEST(single_dimension_tests, are_executor_exception_resolveable) {
  auto executor = [&](auto&& work) {
    std::forward<decltype(work)>(work).set_exception(supply_test_exception());
  };

  ASSERT_ASYNC_EXCEPTION_RESULT(async_on(
                                    [] {
                                      FAIL(); //
                                    },
                                    executor),
                                get_test_exception_proto());

  ASSERT_ASYNC_EXCEPTION_RESULT(this->supply().then(
                                    [] {
                                      FAIL(); //
                                    },
                                    executor),
                                get_test_exception_proto());
}

TYPED_TEST(single_dimension_tests, are_executor_exception_resolveable_erased) {
  auto executor = [&](work work) {
    std::move(work).set_exception(supply_test_exception()); //
  };

  ASSERT_ASYNC_EXCEPTION_RESULT(async_on(
                                    [] {
                                      FAIL(); //
                                    },
                                    executor),
                                get_test_exception_proto());

  ASSERT_ASYNC_EXCEPTION_RESULT(this->supply().then(
                                    [] {
                                      FAIL(); //
                                    },
                                    executor),
                                get_test_exception_proto());
}
