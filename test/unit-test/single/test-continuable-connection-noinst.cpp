
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

static cti::continuable<std::string> http_request(std::string) {
  return cti::make_ready_continuable<std::string>("");
}

struct my_callable {
  void operator()(std::string) && {
    // ...
  }
  void operator()(cti::exception_arg_t, cti::exception_t) && {
    // ...
  }
};

TEST(simple_compilation_tests, error_compile_tests) {
  http_request("github.com").next(my_callable{});

  http_request("github.com") | [](std::string) {
    // ...
    return 0;
  } | [] {
    // ...
  };

  http_request("github.com")
      .then([](std::string) {
        // ...
        return 0;
      })
      .then([](int) {
        // ...
      })
      .fail([](cti::exception_t) {
        // ...
      });

  (http_request("github.com") && http_request("github.com"))
      .then([](std::string, std::string) {
        // ...
      })
      .fail([](cti::exception_t) {
        // ...
      })
      .apply([](auto&& me) {
        // ...
        return std::forward<decltype(me)>(me);
      });

  (http_request("github.com") || http_request("github.com"))
      .then([](std::string) {
        // ...
      })
      .fail([](cti::exception_t) {
        // ...
      });

  (http_request("github.com") >> http_request("github.com"))
      .then([](std::string, std::string) {
        // ...
      })
      .fail([](cti::exception_t) {
        // ...
      });
}

TEST(simple_compilation_tests, connection_compile_tests) {

  cti::when_seq(
      cti::make_ready_continuable(0, 1), 2, //< See this plain value
      cti::populate(cti::make_ready_continuable(3),
                    cti::make_ready_continuable(4)),
      std::make_tuple(std::make_tuple(cti::make_ready_continuable(5))))
      .then([](int r0, int r1, int r2, std::vector<int> r34,
               std::tuple<std::tuple<int>> r5) {
        // ...
        unused(r0, r1, r2, r34, r5);
      });

  auto v = cti::populate(cti::make_ready_continuable(8),
                         cti::make_ready_continuable(9));

  cti::when_seq(v.begin(), v.end()).then([](auto) {
    // ...
  });

  cti::when_seq(cti::make_ready_continuable()) // ...
      .then([] {
        // ...
      });

  cti::when_seq() // ...
      .then([] {
        // ...
      });

  cti::when_seq(cti::make_exceptional_continuable<void>(cti::exception_t{}))
      .fail([](auto) {
        // ...
      });

  cti::when_all(
      cti::make_ready_continuable(0, 1), 2, //< See this plain value
      cti::populate(cti::make_ready_continuable(3),
                    cti::make_ready_continuable(4)),
      std::make_tuple(std::make_tuple(cti::make_ready_continuable(5))))
      .then([](int r0, int r1, int r2, std::vector<int> r34,
               std::tuple<std::tuple<int>> r5) {
        // ...
        unused(r0, r1, r2, r34, r5);
      });

  cti::when_any(cti::make_ready_continuable(22),
                cti::make_ready_continuable(44))
      .then([](int) {
        // ...
      });
}
