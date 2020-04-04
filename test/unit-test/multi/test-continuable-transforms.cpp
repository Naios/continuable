
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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <continuable/transforms/future.hpp>
#include <continuable/continuable.hpp>
#include <test-continuable.hpp>

using namespace cti;
using namespace std::chrono_literals;

template <typename T>
bool is_ready(T& future) {
  // Check that the future is ready
  return future.wait_for(0s) == std::future_status::ready;
}

TYPED_TEST(single_dimension_tests, to_future_test) {
  {
    auto future = this->supply().apply(cti::transforms::to_future());
    ASSERT_TRUE(is_ready(future));
    future.get();
  }

  {
    auto future = this->supply()
                      .then([] {
                        // ...
                        return 0xFD;
                      })
                      .apply(cti::transforms::to_future());

    ASSERT_TRUE(is_ready(future));
    EXPECT_EQ(future.get(), 0xFD);
  }

  {
    auto canary = std::make_tuple(0xFD, 0xF5);

    auto future = this->supply()
                      .then([&] {
                        // ...
                        return canary;
                      })
                      .apply(cti::transforms::to_future());

    ASSERT_TRUE(is_ready(future));
    EXPECT_EQ(future.get(), canary);
  }
}
