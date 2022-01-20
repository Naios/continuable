
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

#include <memory>
#include <utility>
#include <test-continuable.hpp>

using namespace cti;

TEST(single_ready_test, is_not_ready_non_immediate) {
  auto c = async([] {
    // ...
  });

  ASSERT_FALSE(c.is_ready());
}

TEST(single_ready_test, is_not_ready_non_immediate_erasure) {
  continuable<> c = async([] {
    // ...
  });

  ASSERT_FALSE(c.is_ready());
}

TEST(single_ready_test, is_ready_immediate) {
  {
    auto c = make_ready_continuable();
    ASSERT_TRUE(c.is_ready());

    result<> res = std::move(c).unpack();

    ASSERT_TRUE(res.is_value());
  }

  {
    auto c = make_ready_continuable(22);
    ASSERT_TRUE(c.is_ready());

    result<int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 22);
  }

  {
    auto c = make_ready_continuable(33, 44);
    ASSERT_TRUE(c.is_ready());

    result<int, int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 33);
    ASSERT_EQ(get<1>(res), 44);
  }

  {
    auto c = make_ready_continuable(55, 66, 77);
    ASSERT_TRUE(c.is_ready());

    result<int, int, int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 55);
    ASSERT_EQ(get<1>(res), 66);
    ASSERT_EQ(get<2>(res), 77);
  }
}

TEST(single_ready_test, is_ready_immediate_erasure) {
  {
    continuable<> c = make_ready_continuable();

    ASSERT_TRUE(c.is_ready());

    result<> res = std::move(c).unpack();

    ASSERT_TRUE(res.is_value());
  }

  {
    continuable<int> c = make_ready_continuable(22);
    ASSERT_TRUE(c.is_ready());

    result<int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 22);
  }

  {
    continuable<int, int> c = make_ready_continuable(33, 44);
    ASSERT_TRUE(c.is_ready());

    result<int, int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 33);
    ASSERT_EQ(get<1>(res), 44);
  }

  {
    continuable<int, int, int> c = make_ready_continuable(55, 66, 77);
    ASSERT_TRUE(c.is_ready());

    result<int, int, int> res = std::move(c).unpack();

    ASSERT_EQ(get<0>(res), 55);
    ASSERT_EQ(get<1>(res), 66);
    ASSERT_EQ(get<2>(res), 77);
  }
}

TEST(single_ready_test, is_ready_exception) {
  {
    auto c = make_exceptional_continuable<void>(supply_test_exception());
    ASSERT_TRUE(c.is_ready());

    result<> res = std::move(c).unpack();

    ASSERT_TRUE(res.is_exception());

    ASSERT_ASYNC_EXCEPTION_RESULT(
        make_exceptional_continuable<void>(res.get_exception()),
        get_test_exception_proto())
  }

  {
    auto c = make_exceptional_continuable<int, int>(supply_test_exception());
    ASSERT_TRUE(c.is_ready());

    result<int, int> res = std::move(c).unpack();

    ASSERT_ASYNC_EXCEPTION_RESULT(
        make_exceptional_continuable<void>(res.get_exception()),
        get_test_exception_proto())
  }
}

TEST(single_ready_test, is_ready_exception_erasure) {
  {
    continuable<> c =
        make_exceptional_continuable<void>(supply_test_exception());
    ASSERT_TRUE(c.is_ready());

    result<> res = std::move(c).unpack();

    ASSERT_TRUE(res.is_exception());

    ASSERT_ASYNC_EXCEPTION_RESULT(
        make_exceptional_continuable<void>(res.get_exception()),
        get_test_exception_proto())
  }

  {
    continuable<int, int> c =
        make_exceptional_continuable<int, int>(supply_test_exception());
    ASSERT_TRUE(c.is_ready());

    result<int, int> res = std::move(c).unpack();

    ASSERT_ASYNC_EXCEPTION_RESULT(
        make_exceptional_continuable<void>(res.get_exception()),
        get_test_exception_proto())
  }
}
