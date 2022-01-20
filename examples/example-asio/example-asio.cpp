
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

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

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <system_error>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <exception>
#endif

#include <asio.hpp>

#include <continuable/continuable.hpp>

using namespace std::chrono_literals;

inline auto error_code_remapper() {
  return [](auto&& promise, asio::error_code e, auto&&... args) {
    if (e) {
#if defined(CONTINUABLE_HAS_EXCEPTIONS)
      promise.set_exception(std::make_exception_ptr(e));
#else
      promise.set_exception(cti::exception_t(e.value(), e.category()));
#endif
    } else {
      promise.set_value(std::forward<decltype(args)>(args)...);
    }
  };
}

struct functional_io_service {
  asio::io_context service_;
  asio::ip::udp::resolver resolver_;

  functional_io_service() : resolver_(service_) {
  }

  auto trough_post() noexcept {
    return [&](auto&& work) mutable {
      asio::post(service_,
                 [work = std::forward<decltype(work)>(work)]() mutable {
                   std::move(work)();
                   // .. or: work.set_value();
                 });
    };
  }

  auto wait_async(std::chrono::milliseconds /*time*/) {
    return cti::make_continuable<void>([](auto&& promise) {
      // ...
      promise.set_value();
    });
  }

  auto async_resolve(std::string host, std::string service) {
    return cti::promisify<asio::ip::udp::resolver::iterator>::with(
        error_code_remapper(),
        [&](auto&&... args) {
          resolver_.async_resolve(std::forward<decltype(args)>(args)...);
        },
        std::move(host), std::move(service));
  }

  void run() {
    service_.run();
  }
};

int main(int, char**) {
  using asio::ip::udp;

  functional_io_service service;

  service.async_resolve("127.0.0.1", "daytime")
      .then(
          [](udp::resolver::iterator iterator) {
            // ...
            return *iterator;
          },
          service.trough_post())
      .then([](udp::endpoint /*endpoint*/) {
        // auto socket = std::make_shared<udp::socket>(service);
        // socket->async_send_to()
      })
      .fail([](cti::exception_t /*error*/) {
        // ...
      });

  service.run();
  return 0;
}
