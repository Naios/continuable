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

#include "continuable/continuable.hpp"

using namespace cti::detail;
using namespace cti::detail::util;

/// Predicate to check whether an object is callable with the given arguments
template <typename... Args> auto is_invokable_with(identity<Args...>) {
  return [](auto&& callable) {
    (void)callable;
    return is_invokable_t<decltype(callable), identity<Args...>>{};
  };
}

template <typename... Right>
auto reverse(identity<>, identity<Right...> right = identity<>{}) {
  return right;
}
template <typename First, typename... Left, typename... Right>
auto reverse(identity<First, Left...>, identity<Right...> = identity<>{}) {
  return reverse(identity<Left...>{}, identity<First, Right...>{});
}

int main(int, char**) {

  auto cb = [](int, int) { return 0; };

  partial_invoke(cb, 0, 0, 0, 0, 0, 0, 0);

  return 0;
}
