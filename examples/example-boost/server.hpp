
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

#ifndef CONTINUABLE_EXAMPLES_SERVER_HPP_INCLUDED
#define CONTINUABLE_EXAMPLES_SERVER_HPP_INCLUDED

#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <continuable/continuable.hpp>
#include <function2/function2.hpp>

using request_t = boost::beast::http::request<boost::beast::http::string_body>;
using response_t =
    boost::beast::http::response<boost::beast::http::string_body>;

using handler_t =
    fu2::unique_function<cti::continuable<response_t>(request_t const&) const>;

struct http_server {
  virtual ~http_server() = default;

  /// Creates a minimal asynchronous http server using boost asio and beast
  static std::shared_ptr<http_server> create(std::string address,
                                             unsigned short port);

  virtual void listen(handler_t);

  virtual boost::asio::io_context& context();

  virtual void stop();
};

#endif // CONTINUABLE_EXAMPLES_SERVER_HPP_INCLUDED
