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

using cti::detail::util::unused;

void creating_continuables() {
  auto void_continuable = cti::make_continuable<void>([](auto&& callback) {
    //                                          ^^^^

    // Call the callback later when you have finished your work
    callback();
  });

  auto str_continuable =
      cti::make_continuable<std::string>([](auto&& callback) {
        //                                         ^^^^^^^^^^^
        callback("Hello, World!");
      });
}

struct ResultSet {};
template <typename... Args> void mysql_handle_async_query(Args&&...) {}

auto mysql_query(std::string query) {
  return cti::make_continuable<ResultSet>([query = std::move(query)](
      auto&& callback) mutable {
    // Pass the callback to the handler which calls the callback when finished.
    // Every function accepting callbacks works with continuables.
    mysql_handle_async_query(std::move(query),
                             std::forward<decltype(callback)>(callback));
  });
}

void providing_helper_functions() {
  // You may use the helper function like you would normally do,
  // without using the support methods of the continuable.
  mysql_query("DELETE FROM `users` WHERE `id` = 27361");

  // Or using chaining to handle the result which is covered in the
  // documentation.
  mysql_query("SELECT `id`, `name` FROM users").then([](ResultSet result) {
    // ...
    unused(result);
  });
}

void chaining_continuables() {
  mysql_query("SELECT `id`, `name` FROM `users`")
      .then([](ResultSet users) {
        (void)users;
        // Return the next continuable to process ...
        return mysql_query("SELECT `id` name FROM `sessions`");
      })
      .then([](ResultSet sessions) {
        // ... or pass multiple values to the next callback using tuples or
        // pairs ...
        return std::make_tuple(std::move(sessions), true);
      })
      .then([](ResultSet sessions, bool is_ok) {
        (void)sessions;
        (void)is_ok;
        // ... or pass a single value to the next callback ...
        return 10;
      })
      .then([](auto value) {
        //     ^^^^ Templated callbacks are possible too
        (void)value;
      })
      // ... you may even pass continuables to the `then` method directly:
      .then(mysql_query("SELECT * `statistics`"))
      .then([](ResultSet result) {
        // ...
        (void)result;
      });
}

auto http_request(std::string /*url*/) {
  return cti::make_continuable<std::string>(
      [](auto&& callback) { callback("<html>...</html>"); });
}

void connecting_continuables() {
  // `all` of connections:
  (http_request("github.com") && http_request("travis-ci.org") &&
   http_request("atom.io"))
      .then([](std::string github, std::string travis, std::string atom) {
        // The callback is called with the response of github, travis and atom.
        unused(github, travis, atom);
      });

  // `any` of connections:
  (http_request("github.com") || http_request("travis-ci.org") ||
   http_request("atom.io"))
      .then([](std::string github_or_travis_or_atom) {
        // The callback is called with the first response of either github,
        // travis or atom.
        unused(github_or_travis_or_atom);
      });

  // mixed logical connections:
  (http_request("github.com") &&
   (http_request("travis-ci.org") || http_request("atom.io")))
      .then([](std::string github, std::string travis_or_atom) {
        // The callback is called with the response of github for sure
        // and the second parameter represents the response of travis or atom.
        unused(github, travis_or_atom);
      });

  // There are helper functions for connecting continuables:
  auto all =
      cti::all_of(http_request("github.com"), http_request("travis-ci.org"));
  auto any =
      cti::any_of(http_request("github.com"), http_request("travis-ci.org"));
}

int main() {
  creating_continuables();

  providing_helper_functions();

  chaining_continuables();

  connecting_continuables();
}
