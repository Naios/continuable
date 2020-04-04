
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>
#include <continuable/continuable-traverse-async.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>
#include <test-continuable.hpp>

using std::get;
using std::make_tuple;
using std::tuple;

using cti::async_traverse_complete_tag;
using cti::async_traverse_detach_tag;
using cti::async_traverse_in_place_tag;
using cti::async_traverse_visit_tag;
using cti::traverse_pack_async;
using cti::detail::util::unused;

/// A tag which isn't accepted by any mapper
struct not_accepted_tag {};

template <typename Child>
class async_counter_base : public std::enable_shared_from_this<Child> {
  std::size_t counter_ = 0;

public:
  async_counter_base() = default;

  virtual ~async_counter_base() {
  }

  std::size_t const& counter() const noexcept {
    return counter_;
  }

  std::size_t& counter() noexcept {
    return counter_;
  }
};

template <std::size_t ArgCount>
struct async_increasing_int_sync_visitor
    : async_counter_base<async_increasing_int_sync_visitor<ArgCount>> {
  bool operator()(async_traverse_visit_tag, std::size_t i) {
    EXPECT_EQ(i, this->counter());
    ++this->counter();
    return true;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::size_t i, N&& next) {
    unused(i);
    unused(next);

    // Should never be called!
    EXPECT_TRUE(false);
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& pack) {
    unused(pack);

    EXPECT_EQ(this->counter(), ArgCount);
    ++this->counter();
  }
};

template <std::size_t ArgCount>
struct async_increasing_int_visitor
    : async_counter_base<async_increasing_int_visitor<ArgCount>> {

  bool operator()(async_traverse_visit_tag, std::size_t i) const {
    EXPECT_EQ(i, this->counter());
    return false;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::size_t i, N&& next) {
    unused(i);

    ++this->counter();
    std::forward<N>(next)();
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& pack) {
    unused(pack);

    EXPECT_EQ(this->counter(), ArgCount);
    ++this->counter();
  }
};

template <std::size_t ArgCount>
struct async_increasing_int_interrupted_visitor
    : async_counter_base<async_increasing_int_interrupted_visitor<ArgCount>> {

  bool operator()(async_traverse_visit_tag, std::size_t i) {
    EXPECT_EQ(i, this->counter());
    ++this->counter();

    // Detach the control flow at the second step
    return i == 0;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::size_t i, N&& next) {
    EXPECT_EQ(i, 1U);
    EXPECT_EQ(this->counter(), 2U);

    // Don't call next here
    unused(next);
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& pack) const {
    unused(pack);

    // Will never be called
    FAIL();
  }
};

template <std::size_t ArgCount, typename... Args>
void test_async_traversal_base(Args&&... args) {
  // Test that every element is traversed in the correct order
  // when we detach the control flow on every visit.
  {
    auto result = traverse_pack_async(
        async_increasing_int_sync_visitor<ArgCount>{}, args...);
    EXPECT_EQ(result->counter(), ArgCount + 1U);
  }

  // Test that every element is traversed in the correct order
  // when we detach the control flow on every visit.
  {
    auto result =
        traverse_pack_async(async_increasing_int_visitor<ArgCount>{}, args...);
    EXPECT_EQ(result->counter(), ArgCount + 1U);
  }

  // Test that the first element is traversed only,
  // if we don't call the resume continuation.
  {
    auto result = traverse_pack_async(
        async_increasing_int_interrupted_visitor<ArgCount>{}, args...);
    EXPECT_EQ(result->counter(), 2U);
  }
}

TEST(async_traversal, base) {
  // Just test everything using a casual int pack
  test_async_traversal_base<4U>(not_accepted_tag{}, 0U, 1U, not_accepted_tag{},
                                2U, 3U, not_accepted_tag{});
}

template <typename ContainerFactory>
void test_async_container_traversal_impl(ContainerFactory&& container_of) {
  // Test by passing a containers in the middle
  test_async_traversal_base<4U>(0U, container_of(1U, 2U), 3U);
  // Test by splitting the pack in two containers
  test_async_traversal_base<4U>(container_of(0U, 1U), container_of(2U, 3U));
  // Test by passing a huge containers to the traversal
  test_async_traversal_base<4U>(container_of(0U, 1U, 2U, 3U));
}

template <typename T>
struct common_container_factory {
  template <typename... Args>
  T operator()(Args&&... args) {
    return T{std::forward<Args>(args)...};
  }
};

template <typename T>
struct array_container_factory {
  template <typename... Args, typename Array = std::array<T, sizeof...(Args)>>
  Array operator()(Args&&... args) {
    return Array{{std::forward<Args>(args)...}};
  }
};

TEST(async_traversal_container, visit_vector) {
  common_container_factory<std::vector<std::size_t>> factory;
  test_async_container_traversal_impl(factory);
}

TEST(async_traversal_container, visit_list) {
  common_container_factory<std::list<std::size_t>> factory;
  test_async_container_traversal_impl(factory);
}

TEST(async_traversal_container, visit_set) {
  common_container_factory<std::set<std::size_t>> factory;
  test_async_container_traversal_impl(factory);
}

TEST(async_traversal_container, visit_array) {
  array_container_factory<std::size_t> factory;
  test_async_container_traversal_impl(factory);
}

// Test by passing a tuple in the middle
TEST(async_traversal_tuple_like, visit_tuple) {
  test_async_traversal_base<4U>(not_accepted_tag{}, 0U,
                                make_tuple(1U, not_accepted_tag{}, 2U), 3U);
}

// Test by splitting the pack in two tuples
TEST(async_traversal_tuple_like, visit_tuple_nested) {
  test_async_traversal_base<4U>(make_tuple(0U, not_accepted_tag{}, 1U),
                                make_tuple(2U, 3U));
}

// Test by passing a huge tuple to the traversal
TEST(async_traversal_tuple_like, visit_tuple_huge) {
  test_async_traversal_base<4U>(make_tuple(0U, 1U, 2U, 3U));
}

template <typename T, typename... Args,
          typename Vector = std::vector<std::decay_t<T>>>
Vector vector_of(T&& first, Args&&... args) {
  return Vector{std::forward<T>(first), std::forward<Args>(args)...};
}

// Test hierarchies where container and tuple like types are mixed
TEST(async_traversal_mixed_traversal, visit_tuple_container) {
  test_async_traversal_base<4U>(
      0U, make_tuple(std::vector<std::size_t>{1U, 2U}), 3U);
}

TEST(async_traversal_mixed_traversal, visit_mixed_non_accepted) {
  test_async_traversal_base<4U>(
      make_tuple(0U, vector_of(not_accepted_tag{}), vector_of(vector_of(1U))),
      make_tuple(2U, 3U));
}

TEST(async_traversal_mixed_traversal, visit_vector_vector_tuple) {
  test_async_traversal_base<4U>(
      vector_of(vector_of(make_tuple(0U, 1U, 2U, 3U))));
}

template <std::size_t ArgCount>
struct async_unique_sync_visitor
    : async_counter_base<async_unique_sync_visitor<ArgCount>> {

  explicit async_unique_sync_visitor(not_accepted_tag) {
  }

  bool operator()(async_traverse_visit_tag, std::unique_ptr<std::size_t>& i) {
    EXPECT_EQ(*i, this->counter());
    ++this->counter();
    return true;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::unique_ptr<std::size_t>& i,
                  N&& next) {
    unused(i);
    unused(next);

    // Should never be called!
    EXPECT_TRUE(false);
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& pack) {
    unused(pack);

    EXPECT_EQ(this->counter(), ArgCount);
    ++this->counter();
  }
};

template <std::size_t ArgCount>
struct async_unique_visitor
    : async_counter_base<async_unique_visitor<ArgCount>> {

  explicit async_unique_visitor(not_accepted_tag) {
  }

  bool operator()(async_traverse_visit_tag,
                  std::unique_ptr<std::size_t>& i) const {
    EXPECT_EQ(*i, this->counter());
    return false;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::unique_ptr<std::size_t>& i,
                  N&& next) {
    unused(i);

    ++this->counter();
    std::forward<N>(next)();
  }

  template <typename T>
  void operator()(async_traverse_complete_tag, T&& pack) {
    unused(pack);

    EXPECT_EQ(this->counter(), ArgCount);
    ++this->counter();
  }
};

inline auto of(std::size_t i) {
  return std::make_unique<std::size_t>(i);
}

TEST(async_traverse_in_place, construct_inplace_sync) {
  auto result = traverse_pack_async(
      async_traverse_in_place_tag<async_unique_sync_visitor<4>>{},
      not_accepted_tag{}, of(0), of(1), of(2), of(3));
  EXPECT_EQ(result->counter(), 5U);
}

TEST(async_traverse_in_place, construct_inplace_async) {
  auto result = traverse_pack_async(
      async_traverse_in_place_tag<async_unique_visitor<4>>{},
      not_accepted_tag{}, of(0), of(1), of(2), of(3));
  EXPECT_EQ(result->counter(), 5U);
}

struct invalidate_visitor : async_counter_base<invalidate_visitor> {
  bool operator()(async_traverse_visit_tag, std::shared_ptr<int>& i) const {
    EXPECT_EQ(*i, 22);
    return false;
  }

  template <typename N>
  void operator()(async_traverse_detach_tag, std::shared_ptr<int>& i,
                  N&& next) {
    unused(i);

    std::forward<N>(next)();
  }

  // Test whether the passed pack was passed as r-value reference
  void operator()(async_traverse_complete_tag,
                  tuple<std::shared_ptr<int>>&& pack) const {
    // Invalidate the moved object
    tuple<std::shared_ptr<int>> moved = std::move(pack);

    unused(moved);
  }
};

// Check whether the arguments are invalidated (moved out) when called
TEST(async_complete_invalidation, check_whether_frame_released) {
  auto value = std::make_shared<int>(22);

  auto frame = traverse_pack_async(invalidate_visitor{}, value);

  EXPECT_EQ(value.use_count(), 1L);
}
