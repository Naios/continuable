
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

// This file is based on the original boost beast example located at
// http://www.boost.org/doc/libs/1_66_0/libs/beast/example/http/server/async/http_server_async.cpp
// with the following license:
//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#include <server.hpp>

#include <thread>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <function2/function2.hpp>

using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

// Report a failure
void fail(boost::system::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session> {
  tcp::socket socket_;
  handler_t const& callback_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;

public:
  // Take ownership of the socket
  explicit session(tcp::socket socket, handler_t const& callback)
      : socket_(std::move(socket)), callback_(callback),
        strand_(socket_.get_executor()) {
  }

  // Start the asynchronous operation
  void run() {
    do_read();
  }

  void do_read() {
    // Read a request
    http::async_read(
        socket_, buffer_, req_,
        boost::asio::bind_executor(
            strand_, std::bind(&session::on_read, shared_from_this(),
                               std::placeholders::_1, std::placeholders::_2)));
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
      return do_close();

    if (ec)
      return fail(ec, "read");

    // Send the response
    callback_(std::move(req_)).then([this](auto&& msg) {
      // The lifetime of the message has to extend
      // for the duration of the async operation so
      // we use a shared_ptr to manage it.
      auto sp = std::make_shared<std::decay_t<decltype(msg)>>(std::move(msg));

      // Store a type-erased version of the shared
      // pointer in the class to keep it alive.
      res_ = sp;

      // Write the response
      http::async_write(
          socket_, *sp,
          boost::asio::bind_executor(
              strand_, std::bind(&session::on_write, shared_from_this(),
                                 std::placeholders::_1, std::placeholders::_2,
                                 sp->need_eof())));
    });
  }

  void on_write(boost::system::error_code ec, std::size_t bytes_transferred,
                bool close) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
      return fail(ec, "write");

    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      return do_close();
    }

    // We're done with the response so delete it
    res_ = nullptr;

    // Read another request
    do_read();
  }

  void do_close() {
    // Send a TCP shutdown
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
  }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  handler_t callback_;

public:
  listener(boost::asio::io_context& ioc, tcp::endpoint endpoint,
           handler_t&& callback)
      : acceptor_(ioc), socket_(ioc), callback_(std::move(callback)) {
    boost::system::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      fail(ec, "open");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
      fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void run() {
    if (!acceptor_.is_open())
      return;
    do_accept();
  }

  void do_accept() {
    acceptor_.async_accept(socket_,
                           std::bind(&listener::on_accept, shared_from_this(),
                                     std::placeholders::_1));
  }

  void on_accept(boost::system::error_code ec) {
    if (ec) {
      fail(ec, "accept");
    } else {
      // Create the session and run it
      std::make_shared<session>(std::move(socket_), callback_)->run();
    }

    // Accept another connection
    do_accept();
  }
};

//------------------------------------------------------------------------------

struct http_server_impl : http_server
{
  
};

std::shared_ptr<http_server> http_server::create(std::string address, unsigned short port) {
  


  return std::make_shared<>
}


void create_server(std::string address, unsigned short port,
                   handler_t callback) {

  auto const target = boost::asio::ip::make_address(address.c_str());

  // Create and launch a listening port
  std::make_shared<listener>(context, tcp::endpoint{target, port},
                             std::move(callback))
      ->run();
}
