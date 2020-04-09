
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

#include <chrono>
#include <memory>
#include <thread>
#include <continuable/continuable-transforms.hpp>
#include <continuable/continuable.hpp>
#include <continuable/external/asio.hpp>
#include <asio.hpp>
#include <test-continuable.hpp>

using namespace cti;
using namespace std::chrono_literals;

class async_test_helper {
public:
  async_test_helper()
    : context_(1)
    , timer_(context_)
    , work_(std::make_shared<asio::io_context::work>(context_))
    , thread_([&] {
      context_.run();
    }) {}

  ~async_test_helper() {
    assert(work_);
    timer_.cancel();
    work_.reset();
    thread_.join();
  }

  auto wait_for(asio::steady_timer::duration duration) {
    timer_.expires_after(duration);
    return timer_.async_wait(use_continuable);
  }

private:
  asio::io_context context_;
  asio::steady_timer timer_;
  std::shared_ptr<asio::io_context::work> work_;
  std::thread thread_;
};

#ifdef CONTINUABLE_HAS_EXCEPTIONS
TYPED_TEST(single_dimension_tests, wait_test_sync) {
  // We test here whether it deadlocks automatically
  this->supply().apply(cti::transforms::wait());

  ASSERT_EQ(this->supply(36354).apply(cti::transforms::wait()), 36354);

  ASSERT_EQ(this->supply(47463, 3746).apply(cti::transforms::wait()),
            std::make_tuple(47463, 3746));
}

TYPED_TEST(single_dimension_tests, wait_test_async) {
  {
    async_test_helper helper;
    helper.wait_for(50ms).then(this->supply()).apply(cti::transforms::wait());
  }

  {
    async_test_helper helper;
    ASSERT_EQ(helper.wait_for(50ms)
                  .then(this->supply(36354))
                  .apply(cti::transforms::wait()),
              36354);
  }

  {
    async_test_helper helper;
    ASSERT_EQ(helper.wait_for(50ms)
                  .then(this->supply(47463, 3746))
                  .apply(cti::transforms::wait()),
              std::make_tuple(47463, 3746));
  }
}

TYPED_TEST(single_dimension_tests, wait_test_ready) {
  make_ready_continuable().apply(cti::transforms::wait());

  ASSERT_EQ(make_ready_continuable(36354).apply(cti::transforms::wait()),
            36354);

  ASSERT_EQ(make_ready_continuable(47463, 3746).apply(cti::transforms::wait()),
            std::make_tuple(47463, 3746));
}

TYPED_TEST(single_dimension_tests, wait_test_exception) {
  ASSERT_THROW(make_exceptional_continuable<void>(supply_test_exception())
                   .apply(cti::transforms::wait()),
               test_exception);
}

TYPED_TEST(single_dimension_tests, wait_test_cancellation) {
  ASSERT_THROW(make_cancelling_continuable<void>().apply(
                   cti::transforms::wait()),
               transforms::wait_transform_canceled_exception);
}
#endif // CONTINUABLE_HAS_EXCEPTIONS

TYPED_TEST(single_dimension_tests, wait_for_test_sync) {
  this->supply().apply(cti::transforms::wait_for(50ms));

  ASSERT_EQ(
      this->supply(36354).apply(cti::transforms::wait_for(50ms)).get_value(),
      36354);

  ASSERT_EQ(this->supply(47463, 3746)
                .apply(cti::transforms::wait_for(50ms))
                .get_value(),
            std::make_tuple(47463, 3746));

  ASSERT_TRUE(make_continuable<void>([](auto&&) {})
                  .apply(cti::transforms::wait_for(50ms))
                  .is_empty());
}

TYPED_TEST(single_dimension_tests, wait_for_test_async) {
  async_test_helper helper;
  result<> res = helper.wait_for(500ms).apply(cti::transforms::wait_for(50ms));
  ASSERT_FALSE(res.is_exception());
}
