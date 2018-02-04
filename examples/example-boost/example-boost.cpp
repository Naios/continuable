
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

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

#include <server.hpp>

#include <csignal>
#include <functional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio.hpp>

#include <continuable/continuable.hpp>

int main(int, char**) {
  namespace http = boost::beast::http;

  auto server = http_server::create("0.0.0.0", 8080);

  boost::asio::deadline_timer  t

  server->listen([](request_t const& req) -> cti::continuable<response_t> {
    return cti::make_continuable<response_t>([&](auto&& promise) {
      http::response<http::string_body> res{http::status::ok, req.version()};

      // Build the response
      res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, "text/html");
      res.keep_alive(req.keep_alive());
      res.body() = "Hello World!";
      res.prepare_payload();

      // Resolve the request asynchronously
      promise.set_value(res);
    });
  });

  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(context);
  signals.add(SIGTERM);
  signals.add(SIGINT);

  // Run the I/O service on the requested number of threads
  {
    auto const threads = std::thread::hardware_concurrency();
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
      v.emplace_back([&context] { context.run(); });
    context.run();
  }

  return 0;
}
