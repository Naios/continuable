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
#include <system_error>

#include <continuable/continuable.hpp>

static cti::continuable<std::string> http_request(std::string url) {
  return [url = std::move(url)](cti::promise<std::string> promise) {
    if (false) {
      promise.set_value("");
      promise("");
    }
    promise.set_exception(std::error_condition{});
  };
}

/*
 // TODO Fix this
static cti::continuable<std::string> http_request(std::string url) {
  return [url = std::move(url)](auto promise) {
    if (false) {
      promise.set_value("");
      promise("");
    }
    promise.set_exception(std::error_condition{});
  };
}
*/

static auto http_request2(std::string url) {
  return cti::make_continuable<std::string>(
      // ...
      [url = std::move(url)](auto&& promise) {
        if (false) {
          promise.set_value("");
          promise("");
        }
        promise.set_exception(std::error_condition{});
      });
}

int main(int, char**) {
  http_request("github.com")
      .then([](std::string) {
        // ...
      })
      .catching([](std::error_condition) {
        // ...
      });

  /*http_request2("github.com")
      .then([](std::string) {
        // ...
      })
      .catching([](std::error_condition) {
        // ...
      });*/

  return 0;
}
