
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

using namespace cti;
using namespace cti::detail;

TYPED_TEST(single_dimension_tests, are_called_on_destruct) {
  {
    auto allowed = false;

    // Are not supplyd until destruction
    auto continuable = this->supply().then([&] { ASSERT_TRUE(allowed); });
    ASSERT_FALSE(allowed);

    allowed = true;
    EXPECT_ASYNC_COMPLETION(std::move(continuable));
  }

  EXPECT_ASYNC_RESULT(this->supply());

  EXPECT_ASYNC_RESULT(this->supply(0xDA), 0xDA);

  ASSERT_ASYNC_TYPES(this->supply(tag1{}), tag1);
}

template <typename T> auto create_incomplete(T* me) {
  return me->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
    // Destruct the callback here
    auto destroy = std::forward<decltype(callback)>(callback);
    (void)destroy;
  });
}

template <typename T> auto assert_invocation(T* me) {
  return me->make(identity<>{}, identity<void>{},
                  [](auto&& /*callback*/) mutable { FAIL(); });
}

TYPED_TEST(single_dimension_tests, are_incomplete_when_frozen) {
  {
    auto chain = this->supply();
    chain.freeze();
    EXPECT_ASYNC_INCOMPLETE(std::move(chain));
  }

  {
    auto chain = this->supply();
    chain.freeze();
    EXPECT_ASYNC_INCOMPLETE(std::move(chain).then(this->supply()));
  }
}

TYPED_TEST(single_dimension_tests, are_not_dispatched_when_frozen) {
  auto chain = assert_invocation(this);
  chain.freeze();
  EXPECT_ASYNC_INCOMPLETE(std::move(chain));
}

TYPED_TEST(single_dimension_tests, are_not_finished_when_not_continued) {
  {
    auto chain = create_incomplete(this);
    EXPECT_ASYNC_INCOMPLETE(std::move(chain));
  }

  {
    auto chain = create_incomplete(this);
    EXPECT_ASYNC_INCOMPLETE(std::move(chain).then(this->supply()));
  }
}

TYPED_TEST(single_dimension_tests, are_chainable) {
  EXPECT_ASYNC_RESULT(this->supply().then([] {
    return; // void
  }));

  // Type chain
  {
    auto chain = this->supply().then([] { return tag1{}; });
    ASSERT_ASYNC_TYPES(std::move(chain), tag1);
  }

  // Pair chain
  {
    auto chain = this->supply().then([] {
      // ...
      return std::make_pair(tag1{}, tag2{});
    });
    ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2);
  }

  // Tuple chain
  {
    auto chain = this->supply().then([] {
      // ...
      return std::make_tuple(tag1{}, tag2{}, tag3{});
    });
    ASSERT_ASYNC_TYPES(std::move(chain), tag1, tag2, tag3);
  }

  // Erasing chain
  {
    auto chain = this->supply().then(this->supply(tag1{}));
    ASSERT_ASYNC_TYPES(std::move(chain), tag1);
  }

  // Continuing chain
  {
    auto chain = this->supply().then([&] { return this->supply(tag1{}); });

    ASSERT_ASYNC_TYPES(std::move(chain), tag1);
  }
}

TYPED_TEST(single_dimension_tests, are_convertible_to_futures) {
  auto is_ready = [](auto& future) {
    // Check that the future is ready
    return future.wait_for(std::chrono::seconds(0)) ==
           std::future_status::ready;
  };

  {
    auto future = this->supply().futurize();
    ASSERT_TRUE(is_ready(future));
    future.get();
  }

  {
    auto future = this->supply()
                      .then([] {
                        // ...
                        return 0xFD;
                      })
                      .futurize();

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
                      .futurize();

    ASSERT_TRUE(is_ready(future));
    EXPECT_EQ(future.get(), canary);
  }
}

TYPED_TEST(single_dimension_tests, are_partial_callable) {
  EXPECT_ASYNC_RESULT(this->supply(1, 2).then([] {
    // ...
  }));

  EXPECT_ASYNC_RESULT(this->supply(0xDF, 0xDD, 3, 4).then([](int a, int b) {
    EXPECT_EQ(a, 0xDF);
    EXPECT_EQ(b, 0xDD);
  }));
}
