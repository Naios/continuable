
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

using namespace cti;
using namespace cti::detail;

TYPED_TEST(single_dimension_tests, are_called_on_destruct) {
  {
    auto allowed = false;

    // Are not supplyd until destruction
    auto continuable = this->supply().then([&] {
      ASSERT_TRUE(allowed);
    });
    ASSERT_FALSE(allowed);

    allowed = true;
    ASSERT_ASYNC_COMPLETION(std::move(continuable));
  }

  EXPECT_ASYNC_RESULT(this->supply());

  EXPECT_ASYNC_RESULT(this->supply(0xDA), 0xDA);

  ASSERT_ASYNC_TYPES(this->supply(tag1{}), tag1);
}

template <typename T>
auto create_incomplete(T* me) {
  return me->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
    EXPECT_TRUE(callback);
    // Destruct the callback here
    auto destroy = std::forward<decltype(callback)>(callback);
    (void)destroy;
  });
}

template <typename T>
auto create_cancellation(T* me) {
  return me->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
    EXPECT_TRUE(callback);
    make_cancelling_continuable<void>().next(
        std::forward<decltype(callback)>(callback));
  });
}

template <typename T>
auto assert_invocation(T* me) {
  return me->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
    EXPECT_TRUE(callback);
    FAIL();
  });
}

TYPED_TEST(single_dimension_tests, are_incomplete_when_frozen) {
  {
    auto chain = this->supply();
    chain.freeze();
    ASSERT_ASYNC_INCOMPLETION(std::move(chain));
  }

  {
    auto chain = this->supply();
    chain.freeze();
    ASSERT_ASYNC_INCOMPLETION(std::move(chain).then(this->supply()));
  }
}

TYPED_TEST(single_dimension_tests, are_not_dispatched_when_frozen) {
  auto chain = assert_invocation(this);
  chain.freeze();
  ASSERT_ASYNC_INCOMPLETION(std::move(chain));
}

TYPED_TEST(single_dimension_tests, are_not_finished_when_not_continued) {
  {
    auto chain = create_incomplete(this);
    ASSERT_ASYNC_INCOMPLETION(std::move(chain));
  }

  {
    auto chain = create_incomplete(this);
    ASSERT_ASYNC_INCOMPLETION(std::move(chain).then(this->supply()));
  }

  {
    ASSERT_ASYNC_INCOMPLETION(this->supply().then([] {
      return empty_result();
    }));
  }

  {
    ASSERT_ASYNC_INCOMPLETION(this->supply().then([] {
      return stop();
    }));
  }
}

TYPED_TEST(single_dimension_tests, are_not_finished_when_cancelling) {
  {
    auto chain = create_cancellation(this);
    ASSERT_ASYNC_CANCELLATION(std::move(chain));
  }

  {
    auto chain = create_cancellation(this);
    ASSERT_ASYNC_CANCELLATION(std::move(chain).then(this->supply()));
  }
}

TYPED_TEST(single_dimension_tests, are_not_finished_when_cancelling_hook) {
  {
    ASSERT_ASYNC_CANCELLATION(
        this->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
          std::forward<decltype(callback)>(callback).set_canceled();
        }));
  }

  {
    ASSERT_ASYNC_CANCELLATION(
        this->make(identity<>{}, identity<void>{}, [](auto&& callback) mutable {
          std::forward<decltype(callback)>(callback).set_exception({});
        }));
  }

  {
    ASSERT_ASYNC_CANCELLATION(this->supply().then([] {
      return exceptional_result(exception_t{});
    }));
  }

  {
    ASSERT_ASYNC_CANCELLATION(this->supply().then([]() -> result<> {
      return exceptional_result(exception_t{});
    }));
  }

  {
    ASSERT_ASYNC_CANCELLATION(this->supply().then([] {
      return cancellation_result();
    }));
  }

  {
    ASSERT_ASYNC_CANCELLATION(this->supply().then([]() -> result<> {
      return cancellation_result();
    }));
  }
}

TYPED_TEST(single_dimension_tests, freeze_is_kept_across_the_chain) {
  {
    auto chain = this->supply().freeze().then([=] {
      return this->supply();
    });
    ASSERT_TRUE(chain.is_frozen());
  }

  {
    auto chain = this->supply().freeze().then(this->supply());
    ASSERT_TRUE(chain.is_frozen());
  }
}
