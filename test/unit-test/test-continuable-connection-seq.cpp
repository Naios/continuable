
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

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable) {
  auto chain = this->supply() >> this->supply();
  EXPECT_ASYNC_RESULT(std::move(chain));
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_value) {
  auto chain = this->supply(1) >> this->supply(2);
  EXPECT_ASYNC_RESULT(std::move(chain), 1, 2);
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_when_seq) {
  auto chain = cti::when_seq(this->supply(1), this->supply(2), this->supply(3));
  EXPECT_ASYNC_RESULT(std::move(chain), 1, 2, 3);
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_duplicated_val) {
  auto chain = this->supply(1, 2) >> this->supply(3, 4);
  EXPECT_ASYNC_RESULT(std::move(chain), 1, 2, 3, 4);
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_tag) {
  auto chain = this->supply(tag1{}, tag2{}) >> this->supply(tag1{}, tag2{});
  ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2, tag1, tag2);
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_three_tags) {
  auto chain = this->supply(tag1{}, tag2{}, tag3{}) >>
               this->supply(tag1{}, tag2{}, tag3{});
  ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2, tag3, tag1, tag2, tag3);
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_composed) {
  // Check the evaluation order
  unsigned i = 0;
  auto composed =
      make_step(this, i, 0) >> make_step(this, i, 1) >> make_step(this, i, 2);
  EXPECT_ASYNC_RESULT(std::move(composed));
}

TYPED_TEST(single_dimension_tests, is_logical_seq_connectable_chars) {
  auto chain = this->supply('a') >> this->supply('b') >> this->supply('c');
  EXPECT_ASYNC_RESULT(std::move(chain), 'a', 'b', 'c');
}
