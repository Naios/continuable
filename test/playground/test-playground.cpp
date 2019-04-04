/*
  Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>

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

#include <exception>
#include <vector>
#include <continuable/continuable.hpp>

using namespace cti;

continuable<std::string> http_request(std::string /*url*/) {
  return async([]() -> std::string {
    throw std::exception{}; //
  });
}

struct ResultSet {};
struct Buffer {};

continuable<ResultSet> mysql_query(std::string /*url*/) {
  return make_ready_continuable(ResultSet{});
}

continuable<Buffer> read_file(std::string /*url*/) {
  return make_ready_continuable(Buffer{});
}

struct exception_trait {};

struct unhandled_exception_trait {};

struct stacktrace_trait {
  using stacktrace_type = std::vector<void const*>;

  static stacktrace_type gather(std::size_t offset) noexcept {
    return {};
  }

  static exception_t annotate(stacktrace_type stacktrace,
                              exception_t&& exception) noexcept {
    return exception;
  }
};

int main(int, char**) {
  when_all(http_request("github.com"), http_request("atom.io"))
      .then([](std::string /*github*/, std::string /*atom*/) {
        // ...
        return mysql_query("select * from `users`");
      })
      .then([](ResultSet /*result*/) {
        // ...
      });
}
