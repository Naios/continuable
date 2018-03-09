
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

#ifndef TEST_CONTINUABLE_CONNECTION_HPP__
#define TEST_CONTINUABLE_CONNECTION_HPP__

#include <tuple>
#include <type_traits>
#include <vector>

#include <test-continuable.hpp>

template <typename Supplier, typename OperatorConnector>
void test_all_seq_op(Supplier&& supply, OperatorConnector&& op) {
  {
    auto chain = op(supply(), supply());
    EXPECT_ASYNC_RESULT(std::move(chain));
  }

  {
    auto chain = op(supply(1), supply(2));
    EXPECT_ASYNC_RESULT(std::move(chain), 1, 2);
  }

  {
    auto chain = op(supply(1, 2), supply(3, 4, 5));
    EXPECT_ASYNC_RESULT(std::move(chain), 1, 2, 3, 4, 5);
  }

  {
    auto chain = op(supply(tag1{}), supply(tag2{}, tag3{}));
    ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2, tag3);
  }
}

class non_default_constructible {
public:
  explicit non_default_constructible(int) {
  }
};

template <typename Supplier, typename AggregateConnector>
void test_all_seq_aggregate(Supplier&& supply, AggregateConnector&& ag) {
  {
    auto chain = ag(supply(1, 2), supply(3, 4), supply(5, 6));
    EXPECT_ASYNC_RESULT(std::move(chain), 1, 2, 3, 4, 5, 6);
  }

  {
    auto chain =
        ag(std::make_tuple(supply(1, 2), std::make_tuple(supply(3, 4))),
           supply(5, 6));
    EXPECT_ASYNC_RESULT(std::move(chain),
                        std::make_tuple(1, 2, std::make_tuple(3, 4)), 5, 6);
  }

  {
    auto chain = ag(std::make_tuple(
        cti::populate(supply(1, 2), supply(3, 4), supply(5, 6))));
    EXPECT_ASYNC_RESULT(std::move(chain),
                        std::make_tuple(std::vector<std::tuple<int, int>>{
                            {1, 2}, {3, 4}, {5, 6}}));
  }

  {
    auto chain =
        ag(supply(1, 2), supply(non_default_constructible{1}), supply(5, 6));
    ASSERT_ASYNC_TYPES(std::move(chain), int, int, non_default_constructible,
                       int, int);
  }
}

#endif // TEST_CONTINUABLE_CONNECTION_HPP__
