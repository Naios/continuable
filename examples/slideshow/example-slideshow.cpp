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

cti::continuable<std::string> http_request(std::string /*url*/) {
  return cti::make_continuable<std::string>(
      [](auto&& callback) { callback("<html>...</html>"); });
}

struct ResultSet {};
struct Buffer {};

cti::continuable<ResultSet> mysql_query(std::string /*url*/) {
  return cti::make_continuable<std::string>(
      [](auto&& callback) { callback("<html>...</html>"); });
}

cti::continuable<Buffer> read_file(std::string /*url*/) {
  return cti::make_continuable<std::string>(
      [](auto&& callback) { callback("<html>...</html>"); });
}

struct a {
  auto post() const {
    return [](auto&&) {};
  }
};

int main(int, char**) {
  a e;
  auto executor = &e;
  // clang-format off


  // ----------

  (http_request("github.com") && http_request("atom.io"))
    .then([] (std::string github, std::string atom) {
      // ...
      return mysql_query("select * from `users`");
    })
    .then([] (ResultSet result) {
      // ...
    }, executor->post());

  // ----------

  auto cq =      http_request("github.com") && http_request("atom.io")         ;



  auto cq =      http_request("github.com") || http_request("atom.io")         ;



  auto cq =      http_request("github.com") >> http_request("atom.io")         ;

  // ----------

  read_file("entries.csv")
    .then([] (Buffer buffer) {
      // ...
      return std::make_tuple("hey", true, 0);
    })
    .then([] (std::string msg) {
      // ...
    });

  // ----------

  // clang-format on
  return 0;
}
