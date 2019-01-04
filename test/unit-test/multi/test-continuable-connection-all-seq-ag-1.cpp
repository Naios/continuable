
/*
  Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>

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

#include <tuple>
#include <type_traits>
#include <vector>

#include <test-continuable.hpp>

TYPED_TEST(single_aggregate_tests, is_logical_seq_connectable_three_tags) {
  auto chain =
      this->ag(this->supply(1, 2), this->supply(3, 4), this->supply(5, 6));
  EXPECT_ASYNC_RESULT(std::move(chain), 1, 2, 3, 4, 5, 6);
}

TYPED_TEST(single_aggregate_tests, is_logical_connectable_nested) {
  auto chain = this->ag(
      std::make_tuple(this->supply(1, 2), std::make_tuple(this->supply(3, 4))),
      this->supply(5, 6));
  EXPECT_ASYNC_RESULT(std::move(chain),
                      std::make_tuple(1, 2, std::make_tuple(3, 4)), 5, 6);
}
