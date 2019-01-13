
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

#ifndef CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED
#define CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
namespace detail {
template <typename T>
struct loop_trait {
  static_assert(!std::is_same<T, T>::value,
                "The callable passed to cti::loop must always return a "
                "cti::continuable_base which resolves to a cti::result.");
};

template <typename Continuation, typename... Args>
struct loop_trait<continuable_base<Continuation, //
                                   signature_arg_t<result<Args...>>>> {
  template <typename Callable>
  static auto make(Callable&& callable) {
    return make_continuable<Args...>(std::forward<Callable>(callable));
  }
};
template <typename Continuation>
struct loop_trait<continuable_base<Continuation, signature_arg_t<result<>>>> {
  template <typename Callable>
  static auto make(Callable&& callable) {
    return make_continuable<void>(std::forward<Callable>(callable));
  }
};

namespace operations {
template <typename Promise, typename Callable, typename ArgsTuple>
class loop_frame : public std::enable_shared_from_this<
                       loop_frame<Promise, Callable, ArgsTuple>> {
  Promise promise_;
  Callable callable_;
  ArgsTuple args_;

public:
  void loop() {
    traits::unpack(
        [&callable_](auto&&... args) {
          util::invoke(callable_, std::forward<decltype(args)>(args)...)
              .then([me = this->shared_from_this()](auto&& result) {
                if (result.is_empty()) {
                  me->loop();
                } else if (result.is_value()) {
                  traits::unpack(std::move(promise_), result);
                } else {
                  assert(result.is_exception());
                  traits::unpack(std::move(promise_), exception_arg_t{},
                                 result.get_exception());
                }
              });
        },
        args_);
  }
};

template <typename Promise, typename Callable, typename ArgsTuple>
auto make_loop_frame(Promise&& promise, Callable&& callable,
                     ArgsTuple&& args_tuple) {
  using frame_t =
      loop_frame<traits::unrefcv_t<Promise>, traits::unrefcv_t<Callable>,
                 traits::unrefcv_t<ArgsTuple>>;

  return std::make_shared<frame_t>(std::forward<Promise>(promise),
                                   std::forward<Callable>(callable),
                                   std::forward<ArgsTuple>(args_tuple));
}

template <typename Callable, typename... Args>
auto loop(Callable&& callable, Args&&... args) {
  using invocation_result_t =
      decltype(util::invoke(callable, args...).finish());
  using trait_t = loop_trait<invocation_result_t>;
  return trait_t::make([callable = std::forward<decltype(callable)>(callable),
                        args = std::make_tuple(std::forward<decltype(args)>(
                            args)...)](auto&& promise) mutable {
    // Do the actual looping
    auto frame = make_loop_frame(std::forward<decltype(promise)>(promise));
    frame->loop();
  });
}
} // namespace operations
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED
