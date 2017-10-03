
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

#include <string>

#include <continuable/detail/features.hpp>

#include "test-continuable.hpp"

#if !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
struct test_exception : std::exception {
  explicit test_exception() {
  }

  bool operator==(test_exception const&) const noexcept {
    return true;
  }
};

static auto get_test_exception_proto() {
  return test_exception{};
}

static auto supply_test_exception() {
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

static auto get_test_exception_proto() {
  static const my_error_category cat;
  return std::error_condition(123, cat);
}

static auto supply_test_exception() {
  return get_test_exception_proto();
}
#endif

TYPED_TEST(single_dimension_tests, are_using_errors) {
  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception()));
}
