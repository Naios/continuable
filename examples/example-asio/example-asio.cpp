
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

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <system_error>

#include <asio.hpp>

#include <continuable/continuable.hpp>

using namespace std::chrono_literals;

struct functional_io_service : asio::io_service {
  asio::ip::udp::resolver resolver_;

  functional_io_service() : resolver_(*this) {
  }

  auto post() const noexcept {
    return [this](auto&& work) {
      asio::io_service::post(std::forward<decltype(work)>(work));
    };
  }

  auto wait_async(std::chrono::milliseconds time) {
    return cti::make_continuable<void>([](auto&& promise) {
      // ...
      promise.set_value();
    });
  }

  template <typename Protocol>
  auto async_resolve(Protocol&& protocol, std::string host,
                     std::string service) {
    using asio::ip::udp;

    return cti::make_continuable<udp::resolver::iterator>([
      this, protocol = std::forward<Protocol>(protocol), host = std::move(host),
      service = std::move(service)
    ](auto&& promise) mutable {
      resolver_.async_resolve(
          host, service,
          [promise = std::forward<decltype(promise)>(promise)](
              std::error_code const& error,
              udp::resolver::iterator iterator) mutable {

            if (error) {
              promise.set_exception(
                  std::error_condition(error.value(), error.category()));
            } else {
              promise.set_value(std::move(iterator));
            }
          });
    });
  }
};

int main(int, char**) {
  using asio::ip::udp;

  functional_io_service service;

  service.async_resolve(udp::v4(), "127.0.0.1", "daytime")
      .then([](udp::resolver::iterator iterator) {
        // ...
        return *iterator;
      })
      .then([](udp::endpoint endpoint) {
        // auto socket = std::make_shared<udp::socket>(service);
        // socket->async_send_to()
      })
      .fail([](cti::error_type /*error*/) {
        // ...
      });

  service.run();
  return 0;
}
