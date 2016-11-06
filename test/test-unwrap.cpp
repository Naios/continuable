/**
* Copyright 2016 Denis Blank <denis.blank@outlook.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <functional>
#include <typeinfo>

#include "gtest/gtest.h"
#include "continuable/continuable.hpp"

/// Contains helper classes to retrieve the arguments a callback
/// is called with when passing it to the given function.
namespace unwrap {
  
} // namespace unwrap


/*template<typename Expression, typename = void_t<>>
struct is_compiling
	: std::false_type { };

template<typename Expression>
struct is_compiling<Expression, void_t<
  decltype(std::declval<Expression>()(0))>
> : std::true_type { };*/

/*
template<typename T>
struct CanUnwrap

template<typename T>
using CanUnwrapT =  
*/

template<typename Functional, typename... Args>
using is_argument_hint = Identity<
  typename unwrap_continuation_t<Functional>::argument_type,
  Identity<Args...>
>;

#define EXPECT_ARGUMENTS(FUNCTIONAL, ...) \
   \
    auto functional = FUNCTIONAL; \
    using id = Identity<__VA_ARGS__>; \
    id iiii{}; \
    Identity<is_argument_hint<decltype(functional), __VA_ARGS__ >> argst; /*\
    auto result = is_argument_hint< \
      decltype(functional), __VA_ARGS__ \
    >::value; \
    EXPECT_TRUE(result); \*/
  

TEST(ContinuableUnwrapTests, FunctionsAreUnwrappable) {

  auto fn = [](std::function<void(int, int)>) {

  };

  using tt = typename unwrap_continuation_t<decltype(fn)>::argument_type;

  using tttt = is_argument_hint<decltype(fn), int, int>;

  // auto t = tttt::value;

  // EXPECT_ARGUMENTS(fn, int, int);

  // tt arg;

  // auto tp = typeid(arg).name();

  /*
  auto op = &decltype(fn)::operator();

  using opt = decltype(op);

  using unwrp = typename UnwrapArguments<opt>::type::only_argument_type;

  using cu = unwrap_functional_t<decltype(fn)>::type::only_argument_type;

  // using unwrp2 = typename UnwrapArguments<unwrp>::type;

  auto u = unwrp{};

  ;

  int i = 0;

  // auto t = unwrap_functional<decltype(fn)>;
  */

  /*EXPECT_ARGUMENTS();*/

  int i = 0;
  (void)i;
}
