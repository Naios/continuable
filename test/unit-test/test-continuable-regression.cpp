
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

#include <functional>

using namespace cti;
using namespace cti::detail;

TEST(regression_tests, are_multiple_args_mergeable) {
  {
    auto tp = std::make_tuple(1, 2, 3);
    util::merge(tp, tp, tp, tp, tp);
  }

  auto tp2 =
      util::merge(std::make_tuple(), std::make_tuple(1), std::make_tuple(1, 2),
                  std::make_tuple(1, 2, 3), std::make_tuple(1, 2, 3, 4));

  auto count = util::unpack(
      tp2, [](auto... args) { return util::fold(std::plus<int>{}, args...); });
  EXPECT_EQ(count, 20);
}

TEST(recursion_tests, are_noncopyable_mergeable) {
  std::tuple<util::non_copyable> nc1, nc2, nc3;
  util::merge(std::move(nc1), std::move(nc2), std::move(nc3));
}
