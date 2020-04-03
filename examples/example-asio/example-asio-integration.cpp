
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

#include <asio.hpp>

#include <continuable/continuable.hpp>
#include <continuable/external/asio.hpp>

// Queries the NIST daytime service and prints the current date and time
void daytime_service();

// Checks that a timer async_wait returns successfully
void successful_async_wait();

// Checks that a cancelled timer async_wait fails with
// `asio::error::operation_aborted` and is converted to a default constructed
// cti::exception_t.
void cancelled_async_wait();

// Indicates fatal error due to an unexpected failure in the continuation chain.
void unexpected_error(cti::exception_t);

// Check that the failure was an aborted operation, as expected.
void check_aborted_operation(cti::exception_t);

int main(int, char**) {
  daytime_service();

  successful_async_wait();
  cancelled_async_wait();

  return 0;
}

void daytime_service() {
  using asio::ip::tcp;

  asio::io_context ioc(1);
  tcp::resolver resolver(ioc);
  tcp::socket socket(ioc);
  std::string buf;

  resolver.async_resolve("time.nist.gov", "daytime", cti::use_continuable)
      .then([&socket](tcp::resolver::results_type endpoints) {
        return asio::async_connect(socket, endpoints, cti::use_continuable);
      })
      .then([&socket, &buf] {
        return asio::async_read_until(socket, asio::dynamic_buffer(buf), '\n',
                                      cti::use_continuable);
      })
      .then([&buf](std::size_t) {
        puts("Continuation successfully got a daytime response:");
        puts(buf.c_str());
      })
      .fail(&unexpected_error);

  ioc.run();
}

void successful_async_wait() {
  asio::io_context ioc(1);
  asio::steady_timer t(ioc);

  t.expires_after(std::chrono::seconds(1));

  t.async_wait(cti::use_continuable)
      .then([] {
        puts("Continuation succeeded after 1s as expected!");
      })
      .fail(&unexpected_error);

  ioc.run();
}

void cancelled_async_wait() {
  asio::io_context ioc(1);
  asio::steady_timer t(ioc);

  t.expires_after(std::chrono::seconds(999));

  t.async_wait(cti::use_continuable)
      .then([] {
        puts("This should never be called");
        std::terminate();
      })
      .fail(&check_aborted_operation);

  t.cancel_one();
  ioc.run();
}

void unexpected_error(cti::exception_t e) {
  if (!bool(e)) {
    puts("Continuation failed with unexpected cancellation!");
    std::terminate();
  }

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
  try {
    std::rethrow_exception(e);
  } catch (std::exception const& ex) {
    puts("Continuation failed with unexpected exception");
    puts(ex.what());
  } catch (...) {
    // Rethrow the exception to the asynchronous unhandled exception handler
    std::rethrow_exception(std::current_exception());
  }
#else
  puts("Continuation failed with unexpected error");
  puts(e.message().data());
#endif
  std::terminate();
}

void check_aborted_operation(cti::exception_t ex) {
  if (bool(ex)) {
    unexpected_error(ex);
  } else {
    puts("Continuation failed due to aborted async operation, as expected.");
  }
}
