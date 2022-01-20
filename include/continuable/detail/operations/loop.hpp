
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.2.0

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

#ifndef CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED
#define CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED

#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
#include <exception>
#endif // CONTINUABLE_HAS_EXCEPTIONS

namespace cti {
namespace detail {
template <typename T>
struct loop_trait {
  static_assert(!std::is_same<T, T>::value,
                "The callable passed to cti::loop must always return a "
                "cti::continuable_base which resolves to a cti::result.");
};

template <typename... Args>
struct loop_trait<identity<result<Args...>>> {
  template <typename Callable>
  static auto make(Callable&& callable) {
    return make_continuable<Args...>(std::forward<Callable>(callable));
  }
};
template <>
struct loop_trait<identity<result<>>> {
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
  explicit loop_frame(Promise promise, Callable callable, ArgsTuple args)
      : promise_(std::move(promise)), callable_(std::move(callable)),
        args_(std::move(args)) {
  }

  void loop() {
    // MSVC can't evaluate this inside the lambda capture
    auto me = this->shared_from_this();

    traits::unpack(
        [&](auto&&... args) mutable {

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
          try {
#endif // CONTINUABLE_HAS_EXCEPTIONS

            util::invoke(callable_, std::forward<decltype(args)>(args)...)
                .next([me = std::move(me)](auto&&... args) {
                  me->resolve(std::forward<decltype(args)>(args)...);
                });

#if defined(CONTINUABLE_HAS_EXCEPTIONS)
          } catch (...) {
            me->resolve(exception_arg_t{}, std::current_exception());
          }
#endif // CONTINUABLE_HAS_EXCEPTIONS
        },
        args_);
  }

  template <typename Result>
  void resolve(Result&& result) {
    if (result.is_empty()) {
      loop();
    } else if (result.is_value()) {
      traits::unpack(std::move(promise_), std::forward<Result>(result));
    } else {
      assert(result.is_exception());
      std::move(promise_).set_exception(
          std::forward<Result>(result).get_exception());
    }
  }

  void resolve(exception_arg_t, exception_t exception) {
    promise_.set_exception(std::move(exception));
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

  auto constexpr hint = base::annotation_of(identify<invocation_result_t>{});

  using trait_t = loop_trait<std::remove_const_t<decltype(hint)>>;

  return trait_t::make([callable = std::forward<decltype(callable)>(callable),
                        args = std::make_tuple(std::forward<decltype(args)>(
                            args)...)](auto&& promise) mutable {
    // Do the actual looping
    auto frame = make_loop_frame(std::forward<decltype(promise)>(promise),
                                 std::move(callable), std::move(args));
    frame->loop();
  });
}

template <typename Callable, typename Begin, typename End>
auto make_range_looper(Callable&& callable, Begin&& begin, End&& end) {
  return [callable = std::forward<Callable>(callable),
          begin = std::forward<Begin>(begin),
          end = std::forward<End>(end)]() mutable {
    return util::invoke(callable, begin)
        .then([&begin, &end]() mutable -> plain_t<result<>> {
          // begin and end stays valid over the `then` here
          if (++begin != end) {
            return make_plain(result<>::empty());
          } else {
            return make_plain(make_result());
          }
        });
  };
}
} // namespace operations
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_OPERATIONS_LOOP_HPP_INCLUDED
