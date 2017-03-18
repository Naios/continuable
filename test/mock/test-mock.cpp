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

#include <exception>
#include <functional>
#include <string>
#include <system_error>

template <typename... A> struct continuable {
  template <typename T = int> continuable(T&& = 0) {}

  template <typename T> continuable& then(T&&) { return *this; }

  template <typename T> continuable& dropped(T&&) { return *this; }

  template <typename T> continuable& thrown(T&&) { return *this; }

  template <typename T> continuable& failed(T&&) { return *this; }
};

template <typename... A> struct promise {
  void set_value(A...) noexcept {}

  void operator()(A...) && noexcept {}

  void set_exception(std::exception_ptr exception) noexcept {
    // ...
    (void)exception;
  }

  void set_error(std::error_code error) noexcept {
    // ...
    (void)error;
  }

  void cancel() noexcept {}

  bool is_canceled() const noexcept { return false; }
};

template <typename... Result> struct accumulator {
  auto accumulate() {
    return [] {};
  }
};

template <typename Accumulator, typename... Initial>
auto make_accumulator(Accumulator&& /*ac*/, Initial&&... /*initial*/) {
  return std::make_tuple(continuable<>{}, accumulator<int>{});
}

continuable<std::string> http_request(std::string url) {
  return [url = std::move(url)](promise<std::string> result) mutable {
    // ...
    result.set_value("<html>...</html>");
    // ...
    result.set_exception(nullptr);
    // ...
    result.set_error(std::error_code{});
  };
}

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
