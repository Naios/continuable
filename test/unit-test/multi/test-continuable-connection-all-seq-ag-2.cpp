
/*
  Copyright(c) 2015 - 2020 Denis Blank <denis.blank at outlook dot com>

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

#include <tuple>
#include <type_traits>
#include <vector>

#include <test-continuable.hpp>

class non_default_constructible {
public:
  explicit non_default_constructible(int) {
  }
};

TYPED_TEST(single_aggregate_tests, is_logical_connectable_nested_dyn) {
  auto chain = this->ag(std::make_tuple(cti::populate(
      this->supply(1, 2), this->supply(3, 4), this->supply(5, 6))));
  EXPECT_ASYNC_RESULT(std::move(chain),
                      std::make_tuple(std::vector<std::tuple<int, int>>{
                          {1, 2}, {3, 4}, {5, 6}}));
}

TYPED_TEST(single_aggregate_tests, is_logical_connectable_non_default) {
  auto chain =
      this->ag(this->supply(1, 2), this->supply(non_default_constructible{1}),
               this->supply(5, 6));
  ASSERT_ASYNC_TYPES(std::move(chain), int, int, non_default_constructible, int,
                     int);
}
