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

#include <string>
#include <functional>

struct continuable_base {
  template <typename T> continuable_base& then(T&&) { return *this; }

  template <typename T> continuable_base& dropped(T&&) { return *this; }

  template <typename T> continuable_base& thrown(T&&) { return *this; }

  template <typename T> continuable_base& failed(T&&) { return *this; }
};

template<typename... Result>
struct accumulator {
  auto accumulate() { return [] {}; }
};

template <typename Accumulator, typename... Initial>
auto make_accumulator(Accumulator&& /*ac*/, Initial&&... /*initial*/) {
  return std::make_tuple(continuable_base{}, accumulator<int>{});
}

continuable_base http_request(std::string) { return {}; }

int main(int, char**) {
  auto accumulator = make_accumulator(std::plus<int>{}, 0);

  http_request("github.com")
      .then([](std::string response) {
        // ...
        (void)response;
      })
      .then(std::get<1>(accumulator).accumulate())
      .thrown([](std::exception_ptr exception) {
        // ...
        (void)exception;
      })
      .failed([](std::error_code code) {
        // ...
        (void)code;
      });

  return 0;
}
