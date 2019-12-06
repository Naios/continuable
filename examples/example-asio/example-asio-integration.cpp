
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

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

#define ASIO_HAS_RETURN_TYPE_DEDUCTION

#include <continuable/continuable.hpp>
#include <continuable/support/asio.hpp>

#include <asio.hpp>

// Queries the NIST daytime service and prints the current date and time
void daytime_service();

// Checks that a cancelled timer async_wait fails with
// `asio::error::operation_aborted`
void cancelled_async_wait();

int main(int, char**) {
  daytime_service();
  cancelled_async_wait();

  return 0;
}

void daytime_service() {
  using asio::ip::tcp;

  asio::io_context ioc(1);
  tcp::resolver resolver(ioc);
  tcp::socket socket(ioc);
  std::string buf;

  resolver.async_resolve("time.nist.gov", "daytime", asio::use_cti)
      .then([&socket](tcp::resolver::results_type endpoints) {
        return asio::async_connect(socket, endpoints, asio::use_cti);
      })
      .then([&socket, &buf] {
        return asio::async_read_until(socket, asio::dynamic_buffer(buf), '\n',
                                      asio::use_cti);
      })
      .then([&buf](std::size_t) { puts(buf.data()); })
      .fail([](cti::exception_t e) { std::rethrow_exception(e); });

  ioc.run();
}

void cancelled_async_wait() {
  asio::io_context ioc(1);
  asio::steady_timer t(ioc);

  t.expires_after(std::chrono::seconds(999));

  t.async_wait(asio::use_cti)
      .then([] { throw std::logic_error("Unexpected continuation call"); })
      .fail([](cti::exception_t ep) {
        try {
          std::rethrow_exception(ep);
        } catch (asio::system_error const& ex) {
          if (ex.code() == asio::error::operation_aborted) {
            puts("async_wait failed with expected cancellation error");
          } else {
            throw;
          }
        }
      });

  t.cancel_one();
  ioc.run();
}
