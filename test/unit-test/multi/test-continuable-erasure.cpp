
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

#ifndef NO_ERASURE_TESTS

TYPED_TEST(single_dimension_tests, is_eraseable) {

  {
    cti::continuable<int> erasure =
        cti::make_continuable<int>(supplier_of(0xDF));

    EXPECT_ASYNC_RESULT(std::move(erasure), 0xDF);
  }

  {
    cti::continuable<int> erasure = supplier_of(0xDF);

    EXPECT_ASYNC_RESULT(std::move(erasure), 0xDF);
  }

  {
    cti::continuable<int> erasure = this->supply(0xDF);

    EXPECT_ASYNC_RESULT(std::move(erasure), 0xDF);
  }
}

TYPED_TEST(single_dimension_tests, is_callable) {

  cti::continuable<int, int> erased = [](cti::promise<int, int>&& callback) {
    EXPECT_TRUE(callback);
    std::move(callback)(0xDF, 0xDD);
    EXPECT_FALSE(callback);
  };

  EXPECT_ASYNC_RESULT(std::move(erased), 0xDF, 0xDD);
}

#endif // #ifndef NO_ERASURE_TESTS
