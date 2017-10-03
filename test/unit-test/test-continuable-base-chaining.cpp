
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

#include "test-continuable.hpp"

TYPED_TEST(single_dimension_tests, are_chainable) {
  EXPECT_ASYNC_RESULT(this->supply().then([] {
    return; // void
  }));
}

TYPED_TEST(single_dimension_tests, are_type_chainable) {
  auto chain = this->supply().then([] { return tag1{}; });
  ASSERT_ASYNC_TYPES(std::move(chain), tag1);
}

TYPED_TEST(single_dimension_tests, are_pair_chainable) {
  auto chain = this->supply().then([] {
    // ...
    return std::make_pair(tag1{}, tag2{});
  });
  ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2);
}

TYPED_TEST(single_dimension_tests, are_tuple_chainable) {
  auto chain = this->supply().then([] {
    // ...
    return std::make_tuple(tag1{}, tag2{}, tag3{});
  });
  ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2, tag3);
}

TYPED_TEST(single_dimension_tests, are_erasing_chainable) {
  auto chain = this->supply().then(this->supply(tag1{}));
  ASSERT_ASYNC_TYPES(std::move(chain), tag1);
}

TYPED_TEST(single_dimension_tests, are_continuing_chainable) {
  auto chain = this->supply().then([&] { return this->supply(tag1{}); });

  ASSERT_ASYNC_TYPES(std::move(chain), tag1);
}
