/*
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

#include <memory>
#include <thread>
#include <continuable/continuable-transforms.hpp>
#include <continuable/continuable.hpp>
#include <continuable/external/asio.hpp>
#include <asio.hpp>

using namespace cti;
using namespace std::chrono_literals;

int main(int, char**) {
  asio::io_context context(1);
  asio::steady_timer timer(context);
  auto work = std::make_shared<asio::io_context::work>(context);

  timer.expires_after(5s);

  std::thread thread([&] {
    context.run();
    puts("io_context finished");
  });

  result<int> res = timer.async_wait(cti::use_continuable)
                        .then([] {
                          return 1;
                        })
                        .apply(transforms::wait_for(1s));

  assert(res.is_empty());
  puts("async_wait finished");
  work.reset();
  timer.cancel();

  thread.join();
  return 0;
}
