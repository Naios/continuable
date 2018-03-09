
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

#include <algorithm>
#include <array>
#include <functional>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <continuable/continuable-traverse.hpp>

#include <test-continuable.hpp>

using std::get;
using std::make_tuple;
using std::tuple;

using cti::map_pack;
using cti::spread_this;
using cti::traverse_pack;

struct all_map_float {
  template <typename T>
  float operator()(T el) const {
    return float(el + 1.f);
  }
};

struct my_mapper {
  template <typename T, typename std::enable_if<
                            std::is_same<T, int>::value>::type* = nullptr>
  float operator()(T el) const {
    return float(el + 1.f);
  }
};

struct all_map {
  template <typename T>
  int operator()(T) const {
    return 0;
  }
};

TEST(traverse_container, categories) {
  using cti::detail::traversal::container_category_of_t;
  using cti::detail::traversal::container_category_tag;

  static_assert(std::is_same<container_category_tag<false, false>,
                             container_category_of_t<int>>::value,
                "Wrong category!");

  static_assert(std::is_same<container_category_tag<true, false>,
                             container_category_of_t<std::vector<int>>>::value,
                "Wrong category!");

  static_assert(std::is_same<container_category_tag<false, true>,
                             container_category_of_t<std::tuple<int>>>::value,
                "Wrong category!");

  static_assert(
      std::is_same<container_category_tag<true, true>,
                   container_category_of_t<std::array<int, 2>>>::value,
      "Wrong category!");
}

TEST(traverse_mixed, traversal) {
  auto res =
      map_pack(all_map_float{}, 0, 1.f, make_tuple(1.f, 3),
               std::vector<std::vector<int>>{{1, 2}, {4, 5}},
               std::vector<std::vector<float>>{{1.f, 2.f}, {4.f, 5.f}}, 2);

  auto expected = make_tuple( // ...
      1.f, 2.f, make_tuple(2.f, 4.f),
      std::vector<std::vector<float>>{{2.f, 3.f}, {5.f, 6.f}},
      std::vector<std::vector<float>>{{2.f, 3.f}, {5.f, 6.f}}, 3.f);

  static_assert(std::is_same<decltype(res), decltype(expected)>::value,
                "Type mismatch!");
  EXPECT_TRUE((res == expected));
}

TEST(traverse_mixed, build_regression) {
  // Broken build regression tests:
  traverse_pack(my_mapper{}, int(0), 1.f);
  map_pack(all_map{}, 0, std::vector<int>{1, 2});
}

TEST(traverse_mixed, container_container_map) {
  // Also a regression test
  auto res = map_pack(all_map{}, std::vector<std::vector<int>>{{1, 2}});
  EXPECT_EQ((res[0][0]), (0));
}

TEST(traverse_mixed, result_tuple_mapped) {
  auto res = map_pack(
      my_mapper{}, 0, 1.f,
      make_tuple(1.f, 3, std::vector<std::vector<int>>{{1, 2}, {4, 5}},
                 std::vector<std::vector<float>>{{1.f, 2.f}, {4.f, 5.f}}),
      2);

  auto expected = make_tuple( // ...
      1.f, 1.f,
      make_tuple(1.f, 4.f,
                 std::vector<std::vector<float>>{{2.f, 3.f}, {5.f, 6.f}},
                 std::vector<std::vector<float>>{{1.f, 2.f}, {4.f, 5.f}}),
      3.f);

  static_assert(std::is_same<decltype(res), decltype(expected)>::value,
                "Type mismatch!");
  EXPECT_TRUE((res == expected));
}

TEST(traverse_mixed, all_elements_traversed) {
  int count = 0;
  traverse_pack(
      [&](int el) {
        EXPECT_EQ((el), (count + 1));
        count = el;
      },
      1, make_tuple(2, 3, std::vector<std::vector<int>>{{4, 5}, {6, 7}}));

  EXPECT_EQ((count), (7));
}

template <typename T>
struct my_allocator {
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using const_pointer = T const*;
  using reference = T&;
  using const_reference = T const&;

  unsigned state_;

  explicit my_allocator(unsigned state) : state_(state) {
    return;
  }

  template <typename O>
  my_allocator(my_allocator<O> const& other) : state_(other.state_) {
    return;
  }

  template <typename O>
  my_allocator& operator=(my_allocator<O> const& other) {
    state_ = other.state_;
    return *this;
  }

  template <typename O>
  struct rebind {
    using other = my_allocator<O>;
  };

  pointer allocate(size_type n, void const* hint = nullptr) {
    return std::allocator<T>{}.allocate(n, hint);
  }

  void deallocate(pointer p, size_type n) {
    return std::allocator<T>{}.deallocate(p, n);
  }
};

// Traits
TEST(traverse_mixed_container_remap, all_elements_traversed) {
  // TODO Enable this
  // using detail::container_remapping::has_push_back;
  // EXPECT_EQ((has_push_back<std::vector<int>, int>::value), true);
  // EXPECT_EQ((has_push_back<int, int>::value), false);
}

static unsigned long int_remapper(unsigned short i) {
  return i - 1;
}

// Rebinds the values
TEST(traverse_rebind_allocator, maps_values) {
  std::vector<unsigned short> source = {1, 2, 3};
  std::vector<unsigned long> dest = map_pack(int_remapper, source);

  EXPECT_TRUE((dest == decltype(dest){0, 1, 2}));
}

// Rebinds the allocator
TEST(traverse_rebind_allocator, converts_allocator) {
  static unsigned const canary = 78787;

  my_allocator<unsigned short> allocator(canary);
  std::vector<unsigned short, my_allocator<unsigned short>> source(allocator);

  // Empty
  {
    std::vector<unsigned long, my_allocator<unsigned long>> remapped =
        map_pack(int_remapper, source);

    EXPECT_EQ((remapped.get_allocator().state_), (canary));
  }

  // Non empty
  source.push_back(1);
  {
    std::vector<unsigned long, my_allocator<unsigned long>> remapped =
        map_pack(int_remapper, source);

    EXPECT_EQ((remapped.get_allocator().state_), (canary));
  }
}

struct mytester {
  using traversor_type = mytester;

  int operator()(int) {
    return 0;
  }
};

struct my_int_mapper {
  template <typename T, typename std::enable_if<
                            std::is_same<T, int>::value>::type* = nullptr>
  float operator()(T el) const {
    return float(el + 1.f);
  }
};

// TODO Take a look at this... what it was supposed to test
TEST(traverse_mixed_fall_through, misc) {
  traverse_pack(my_int_mapper{}, int(0),
                std::vector<tuple<float, float>>{make_tuple(1.f, 2.f)},
                make_tuple(std::vector<float>{1.f, 2.f}));

  traverse_pack(my_int_mapper{}, int(0),
                std::vector<std::vector<float>>{{1.f, 2.f}},
                make_tuple(1.f, 2.f));

  auto res1 = map_pack(my_int_mapper{}, int(0),
                       std::vector<std::vector<float>>{{1.f, 2.f}},
                       make_tuple(77.f, 2));

  (void)res1;

  auto res2 = map_pack(
      [](int) {
        // ...
        return 0;
      },
      1, std::vector<int>{2, 3});
  (void)res2;
}

class counter_mapper {
  std::reference_wrapper<int> counter_;

public:
  explicit counter_mapper(int& counter) : counter_(counter) {
  }

  template <typename T>
  void operator()(T) const {
    ++counter_.get();
  }
};

struct test_tag_1 {};
struct test_tag_2 {};
struct test_tag_3 {};

class counter_mapper_rejecting_non_tag_1 {
  std::reference_wrapper<int> counter_;

public:
  explicit counter_mapper_rejecting_non_tag_1(int& counter)
      : counter_(counter) {
  }

  void operator()(test_tag_1) {
    ++counter_.get();
  }
};

struct tag_shift_mapper {
  test_tag_2 operator()(test_tag_1) const {
    return {};
  }

  test_tag_3 operator()(test_tag_2) const {
    return {};
  }

  test_tag_1 operator()(test_tag_3) const {
    return {};
  }

  float operator()(int) const {
    return 0.f;
  }
};

class counter_mapper_rejecting_non_tag_1_sfinae {
  std::reference_wrapper<int> counter_;

public:
  explicit counter_mapper_rejecting_non_tag_1_sfinae(int& counter)
      : counter_(counter) {
  }

  template <typename T, typename std::enable_if<
                            std::is_same<typename std::decay<T>::type,
                                         test_tag_1>::value>::type* = nullptr>
  void operator()(T) {
    ++counter_.get();
  }
};

// Every element in the pack is visited
TEST(traverse_strategic_traverse, visit_all_elements) {
  int counter = 0;
  counter_mapper mapper(counter);
  traverse_pack(mapper, test_tag_1{}, test_tag_2{}, test_tag_3{});
  EXPECT_EQ(counter, 3);
}

// Every element in the pack is visited from left to right
TEST(traverse_strategic_traverse, visit_left_to_right) {
  int counter = 0;
  traverse_pack(
      [&](int el) {
        EXPECT_EQ(counter, el);
        ++counter;
      },
      0, 1, 2, 3);
  EXPECT_EQ(counter, 4);
}

// Elements accepted by the mapper aren't traversed:
// - Signature
TEST(traverse_strategic_traverse, visit_non_accepted) {
  int counter = 0;
  counter_mapper_rejecting_non_tag_1 mapper(counter);
  traverse_pack(mapper, test_tag_1{}, test_tag_2{}, test_tag_3{});
  EXPECT_EQ(counter, 1);
}

// - SFINAE
TEST(traverse_strategic_traverse, visit_not_finaed) {
  int counter = 0;
  counter_mapper_rejecting_non_tag_1_sfinae mapper(counter);
  traverse_pack(mapper, test_tag_1{}, test_tag_2{}, test_tag_3{});
  EXPECT_EQ(counter, 1);
}

// Remapping works across values
TEST(traverse_strategic_traverse, remap_across_values) {
  tuple<int, int, int> res = map_pack([](int i) { return i + 1; }, 0, 1, 2);

  auto expected = make_tuple(1, 2, 3);
  EXPECT_TRUE((res == expected));
}

// Remapping works across types
TEST(traverse_strategic_traverse, remap_across_types) {
  tag_shift_mapper mapper;
  tuple<float, test_tag_2, test_tag_3, test_tag_1> res =
      map_pack(mapper, 1, test_tag_1{}, test_tag_2{}, test_tag_3{});

  EXPECT_EQ(get<0>(res), 0.f);
}

// Remapping works with move-only objects
TEST(traverse_strategic_traverse, remap_move_only) {
  std::unique_ptr<int> p1(new int(1));
  std::unique_ptr<int> p2(new int(2));
  std::unique_ptr<int> p3(new int(3));

  tuple<std::unique_ptr<unsigned>, std::unique_ptr<unsigned>,
        std::unique_ptr<unsigned>>
      res = map_pack(
          // Since we pass the unique_ptr's as r-value,
          // those should be passed as r-values to the mapper.
          [](std::unique_ptr<int>&& ptr) {
            // We explicitly move the ownership here
            std::unique_ptr<int> owned = std::move(ptr);
            return std::unique_ptr<unsigned>(new unsigned(*owned + 1));
          },
          std::move(p1), std::move(p2), std::move(p3));

  // We expect the ownership of p1 - p3 to be invalid
  EXPECT_TRUE((!bool(p1)));
  EXPECT_TRUE((!bool(p2)));
  EXPECT_TRUE((!bool(p3)));

  EXPECT_EQ((*get<0>(res)), 2U);
  EXPECT_EQ((*get<1>(res)), 3U);
  EXPECT_EQ((*get<2>(res)), 4U);
}

// Move only types contained in a pack which was passed as l-value
// reference is forwarded to the mapper as reference too.
TEST(traverse_strategic_traverse, remap_references) {
  std::vector<std::unique_ptr<int>> container;
  container.push_back(std::unique_ptr<int>(new int(3)));

  std::vector<int> res =
      map_pack([](std::unique_ptr<int>& p) { return *p; }, container);

  EXPECT_EQ(res.size(), 1U);
  EXPECT_EQ(res[0], 3);
}

// Single object remapping returns the value itself without any boxing
TEST(traverse_strategic_traverse, remap_single_values) {
  int res = map_pack([](int i) { return i; }, 1);
  EXPECT_EQ(res, 1);
}

// Make it possible to pass move only objects in as reference,
// while returning those as reference.
TEST(traverse_strategic_traverse, remap_move_only_as_ref) {
  std::unique_ptr<int> ptr(new int(7));

  std::unique_ptr<int> const& ref = map_pack(
      [](std::unique_ptr<int> const& ref) -> std::unique_ptr<int> const& {
        // ...
        return ref;
      },
      ptr);

  EXPECT_EQ(*ref, 7);
  *ptr = 0;
  EXPECT_EQ(*ref, 0);
}

// Multiple args: Make it possible to pass move only objects in
// as reference, while returning those as reference.
TEST(traverse_strategic_traverse, remap_multiple_move_only_as_ref) {
  std::unique_ptr<int> ptr1(new int(6));
  std::unique_ptr<int> ptr2(new int(7));

  tuple<std::unique_ptr<int> const&, std::unique_ptr<int> const&> ref =
      map_pack(
          [](std::unique_ptr<int> const& ref) -> std::unique_ptr<int> const& {
            // ...
            return ref;
          },
          ptr1, ptr2);

  EXPECT_EQ((*get<0>(ref)), 6);
  EXPECT_EQ((*get<1>(ref)), 7);
  *ptr1 = 1;
  *ptr2 = 2;
  EXPECT_EQ((*get<0>(ref)), 1);
  EXPECT_EQ((*get<1>(ref)), 2);
}

// Every element in the container is visited
// - Plain container
TEST(test_strategic_container_traverse, plain_container) {
  int counter = 0;
  counter_mapper mapper(counter);
  std::vector<int> container;
  container.resize(100);
  traverse_pack(mapper, std::move(container));
  EXPECT_EQ(counter, 100);
}

// - Nested container
TEST(test_strategic_container_traverse, nested_container) {
  int counter = 0;
  counter_mapper mapper(counter);
  std::vector<std::vector<int>> container;
  for (unsigned i = 0; i < 10; ++i) {
    std::vector<int> nested;
    nested.resize(10);
    container.push_back(nested);
  }

  traverse_pack(mapper, std::move(container));
  EXPECT_EQ(counter, 100);
}

// Every element in the container is visited from left to right
TEST(test_strategic_container_traverse, left_to_right) {
  int counter = 0;
  traverse_pack(
      [&](int el) {
        EXPECT_EQ(counter, el);
        ++counter;
      },
      std::vector<int>{0, 1}, std::vector<std::vector<int>>{{2, 3}, {4, 5}});
  EXPECT_EQ(counter, 6);
}

// The container type itself is changed
// - Plain container
TEST(test_strategic_container_traverse, type_changed_plain) {
  std::vector<int> container{1, 2, 3};
  std::vector<float> res =
      map_pack([](int) { return 0.f; }, std::move(container));
  EXPECT_EQ(res.size(), 3U);
}

// - Nested container
TEST(test_strategic_container_traverse, type_changed_nested) {
  std::vector<std::vector<int>> container;
  std::vector<std::vector<float>> res =
      map_pack([](int) { return 0.f; }, std::move(container));
}

// - Move only container
TEST(test_strategic_container_traverse, type_changed_move_only) {
  std::vector<std::unique_ptr<int>> container;
  container.push_back(std::unique_ptr<int>(new int(5)));
  std::vector<int> res = map_pack(
      [](std::unique_ptr<int>&& ptr) { return *ptr; }, std::move(container));

  EXPECT_EQ(res.size(), 1U);
  EXPECT_EQ(res[0], 5);
}

TEST(test_strategic_container_traverse, traverse_move_only_wrapped) {
  std::vector<std::unique_ptr<int>> container;
  container.push_back(std::unique_ptr<int>(new int(5)));

  std::size_t counter = 0;
  traverse_pack(
      [&counter](auto&& ptr) {
        std::unique_ptr<int> moved(std::forward<decltype(ptr)>(ptr));
        EXPECT_EQ((*moved), 5);
        ++counter;
      },
      std::make_tuple(std::move(container)));

  EXPECT_EQ(counter, 1U);
}

// Every element in the container is remapped
// - Plain container
TEST(test_strategic_container_traverse, every_element_remapped_plain) {
  std::vector<int> container(100, 1);
  auto res = map_pack([](int) { return 2; }, std::move(container));

  EXPECT_TRUE(
      (std::all_of(res.begin(), res.end(), [](int i) { return i == 2; })));
}

// - Nested container
TEST(test_strategic_container_traverse, every_element_remapped_nested) {
  std::vector<std::list<int>> container;
  for (unsigned i = 0; i < 10; ++i) {
    std::list<int> nested(10, 1);
    container.push_back(nested);
  }

  auto res = map_pack([](int) { return 2; }, std::move(container));
  EXPECT_TRUE(
      (std::all_of(res.begin(), res.end(), [](std::list<int> const& nested) {
        return std::all_of(nested.begin(), nested.end(),
                           [](int i) { return i == 2; });
      })));
}

/// - Ensure correct container remapping when returning references
// l-value references
TEST(test_strategic_container_traverse, every_element_remapped_lvalue) {
  std::vector<std::unique_ptr<int>> container;
  container.push_back(std::unique_ptr<int>(new int(7)));

  std::vector<int> res = map_pack(
      [](std::unique_ptr<int> const& ref) -> int const& {
        // ...
        return *ref;
      },
      container);

  EXPECT_EQ(res.size(), 1U);
  EXPECT_EQ(res[0], 7);
}

// r-value references
TEST(test_strategic_container_traverse, every_element_remapped_rvalue) {
  std::vector<std::unique_ptr<std::unique_ptr<int>>> container;
  container.push_back(std::unique_ptr<std::unique_ptr<int>>(
      new std::unique_ptr<int>(new int(7))));

  std::vector<std::unique_ptr<int>> res = map_pack(
      [](std::unique_ptr<std::unique_ptr<int>> &
         ref) -> std::unique_ptr<int>&& {
        // ...
        return std::move(*ref);
      },
      container);

  EXPECT_EQ(res.size(), 1U);
  EXPECT_EQ((*res[0]), 7);
}

TEST(traverse_single_test, test_strategic_tuple_like_traverse_homogeneous) {
  // Fixed size homogeneous container
  std::array<int, 3> values{{1, 2, 3}};
  std::array<float, 3> res = map_pack([](int) { return 1.f; }, values);

  EXPECT_TRUE((res == std::array<float, 3>{{1.f, 1.f, 1.f}}));
}

// Every element in the tuple like type is visited
TEST(traverse_strategic_tuple_like_traverse, all_elements_visited) {
  int counter = 0;
  counter_mapper mapper(counter);
  traverse_pack(mapper, make_tuple(test_tag_1{}, test_tag_2{}, test_tag_3{}));
  EXPECT_EQ(counter, 3);
}

// Every element in the tuple like type is visited from left to right
TEST(traverse_strategic_tuple_like_traverse, visited_left_to_right) {
  int counter = 0;
  traverse_pack(
      [&](int el) {
        EXPECT_EQ(counter, el);
        ++counter;
      },
      make_tuple(0, 1), make_tuple(make_tuple(2, 3), make_tuple(4, 5)),
      make_tuple(make_tuple(make_tuple(6, 7))));
  EXPECT_EQ(counter, 8);
}

// The container tuple like type itself is changed
TEST(traverse_strategic_tuple_like_traverse, type_is_remapped) {
  tag_shift_mapper mapper;
  tuple<float, test_tag_2, test_tag_3, test_tag_1> res =
      map_pack(mapper, make_tuple(1, test_tag_1{}, test_tag_2{}, test_tag_3{}));

  EXPECT_EQ(get<0>(res), 0.f);
}

// Every element in the tuple like type is remapped
TEST(traverse_strategic_tuple_like_traverse, all_elements_remapped) {
  tuple<float, float, float> res =
      map_pack([](int) { return 1.f; }, make_tuple(0, 0, 0));

  auto expected = make_tuple(1.f, 1.f, 1.f);

  static_assert(std::is_same<decltype(res), decltype(expected)>::value,
                "Type mismatch!");
  EXPECT_TRUE((res == expected));
}

// Make it possible to pass tuples containing move only objects
// in as reference, while returning those as reference.
TEST(traverse_strategic_tuple_like_traverse, remap_references) {
  auto value = make_tuple(std::unique_ptr<int>(new int(6)),
                          std::unique_ptr<int>(new int(7)));

  tuple<std::unique_ptr<int> const&, std::unique_ptr<int> const&> ref =
      map_pack(
          [](std::unique_ptr<int> const& ref) -> std::unique_ptr<int> const& {
            // ...
            return ref;
          },
          value);

  EXPECT_EQ((*get<0>(ref)), 6);
  EXPECT_EQ((*get<1>(ref)), 7);
  (*get<0>(ref)) = 1;
  (*get<1>(ref)) = 2;
  EXPECT_EQ((*get<0>(ref)), 1);
  EXPECT_EQ((*get<1>(ref)), 2);
}

/// A mapper which duplicates the given element
struct duplicate_mapper {
  template <typename T>
  auto operator()(T arg) -> decltype(spread_this(arg, arg)) {
    return spread_this(arg, arg);
  }
};

/// A mapper which removes the current element
struct zero_mapper {
  template <typename T>
  auto operator()(T) -> decltype(spread_this()) {
    return spread_this();
  }
};

// 1:2 mappings (multiple arguments)
TEST(traverse_spread_traverse, one_to_two_mapping) {
  tuple<int, int, int, int> res = map_pack(duplicate_mapper{}, 1, 2);

  auto expected = make_tuple(1, 1, 2, 2);

  EXPECT_TRUE((res == expected));
}

// 1:0 mappings
TEST(traverse_spread_traverse, one_to_zero_mapping) {
  using Result = decltype(map_pack(zero_mapper{}, 0, 1, 2));
  static_assert(std::is_void<Result>::value, "Failed...");
}

// 1:2 mappings (multiple arguments)
TEST(traverse_spread_container_traverse, one_to_two_mapping) {
  std::vector<tuple<int, int>> res =
      map_pack(duplicate_mapper{}, std::vector<int>{1});

  std::vector<tuple<int, int>> expected;
  expected.push_back(make_tuple(1, 1));

  EXPECT_TRUE((res == expected));
}

// 1:0 mappings
TEST(traverse_spread_container_traverse, one_to_zero_mapping) {
  using Result = decltype(map_pack(zero_mapper{}, std::vector<int>{1}));
  static_assert(std::is_void<Result>::value, "Failed...");
}

// 1:2 mappings (multiple arguments)
TEST(traverse_spread_container_traverse, multiple_args) {
  auto res = map_pack(duplicate_mapper{}, std::array<int, 2>{{1, 2}});

  std::array<int, 4> expected{{1, 1, 2, 2}};

  EXPECT_TRUE((res == expected));
}

// 1:2 mappings (multiple arguments)
TEST(traverse_spread_tuple_like_traverse, multiple_args) {
  tuple<tuple<int, int, int, int>> res =
      map_pack(duplicate_mapper{}, make_tuple(make_tuple(1, 2)));

  tuple<tuple<int, int, int, int>> expected =
      make_tuple(make_tuple(1, 1, 2, 2));

  EXPECT_TRUE((res == expected));
}

// 1:0 mappings
TEST(traverse_spread_tuple_like_traverse, one_to_zero_mapping_tuple) {
  using Result =
      decltype(map_pack(zero_mapper{}, make_tuple(make_tuple(1, 2), 1), 1));
  static_assert(std::is_void<Result>::value, "Failed...");
}

// 1:0 mappings
TEST(traverse_spread_tuple_like_traverse, one_to_zero_mapping_array) {
  using Result = decltype(map_pack(zero_mapper{}, std::array<int, 2>{{1, 2}}));
  static_assert(std::is_void<Result>::value, "Failed...");
}

struct flat_tupelizing_tag1 {};
struct flat_tupelizing_tag2 {};

struct flat_tupelizing_mapper {
  auto operator()(flat_tupelizing_tag1) {
    return 7;
  }
  auto operator()(flat_tupelizing_tag2) {
    return spread_this();
  }
};

TEST(traversal_regressions, flat_tupelizing) {
  {
    std::tuple<int> result =
        map_pack(flat_tupelizing_mapper{}, flat_tupelizing_tag1{},
                 flat_tupelizing_tag2{});
    EXPECT_EQ(std::get<0>(result), 7);
  }

  {
    std::tuple<int> result = map_pack(
        flat_tupelizing_mapper{},
        std::make_tuple(flat_tupelizing_tag1{}, flat_tupelizing_tag2{}));
    EXPECT_EQ(std::get<0>(result), 7);
  }
}
