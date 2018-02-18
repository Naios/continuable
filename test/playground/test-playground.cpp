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

#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include <continuable/continuable.hpp>

static cti::continuable<std::string> http_request(std::string url) {
  return [url = std::move(url)](cti::promise<std::string> promise) {
    if (false) {
      promise.set_value("");
      std::forward<decltype(promise)>(promise)("");
    }
    promise.set_exception(std::error_condition{});
  };
}

static auto http_request2(std::string url) {
  return cti::make_continuable<std::string>(
      // ...
      [url = std::move(url)](auto&& promise) {
        if (false) {
          promise.set_value("");
          std::forward<decltype(promise)>(promise)("");
        }
        promise.set_exception(std::error_condition{});
      });
}

static cti::continuable<std::string> http_request3(std::string url) {
  return [url = std::move(url)](auto&& promise) {
    if (false) {
      promise.set_value("");
      std::forward<decltype(promise)>(promise)("");
    }
    promise.set_exception(std::error_condition{});
  };
}

struct my_callable {
  void operator()(std::string) && {
    // ...
  }
  void operator()(cti::dispatch_error_tag, cti::error_type) && {
    // ...
  }
};

void some_requests() {
  http_request("github.com").next(my_callable{});

  http_request("github.com") | [](std::string) {
    // ...
    return 0;
  } | [] {
    // ...
  };

  http_request2("github.com")
      .then([](std::string) {
        // ...
        return 0;
      })
      .then([](int) {
        // ...
      })
      .fail([](std::error_condition) {
        // ...
      });

  (http_request("github.com") && http_request3("github.com"))
      .then([](std::string, std::string) {
        // ...
      })
      .fail([](std::error_condition) {
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
      .fail([](std::error_condition) {
        // ...
      });

  (http_request("github.com") >> http_request("github.com"))
      .then([](std::string, std::string) {
        // ...
      })
      .fail([](std::error_condition) {
        // ...
      });
}

namespace cti {
namespace detail {

struct c {};

template <typename C, typename... Args>
struct loc {};

struct runtime_insertion {
  std::size_t begin, end;
};

template <typename... Args>
struct future_result {
  std::tuple<Args...> result_;
};

template <std::size_t Begin, std::size_t End>
struct static_insertion {};

template <typename... Hierarchy>
struct indexer_frame {
  std::tuple<Hierarchy...> hierarchy_;

  template <typename T, std::enable_if_t<true>* = nullptr>
  auto operator()(T&& continuable) {
  }
};

struct result_extractor_mapper {
  template <typename T,
            std::enable_if_t<is_continuable<std::decay_t<T>>::value>* = nullptr>
  auto operator()(T&& continuable) {

  }
};

// 0. We create the result pack from the provides values and
//    the async values if those are default constructible,
//    otherwise use a lazy initialization wrapper and unwrap
//    the whole pack when the composition is finished.
//    value -> value, single async value -> single value
//    multiple async value -> tuple of async values.
//
// 1.

} // namespace detail
} // namespace cti

int main(int, char**) {

  using namespace cti::detail;

  std::vector<int> vc{1, 2, 3};

  // std::tuple<c, c, c> t;
  // std::tuple<loc<c, ct<0>>, c, c> loc;

  cti::map_pack([](auto&& /*continuable*/) { return 0; }, vc);

  return 0;
}
