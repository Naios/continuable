
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

#ifndef CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED

#include <type_traits>
#include <utility>
#include <function2/function2.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace erasure {
template <typename... Args>
using callback_erasure_t =
    fu2::function_base<true, false, fu2::capacity_none, true, false,
                       void(Args...)&&, void(exception_arg_t, exception_t) &&>;

#ifdef CONTINUABLE_HAS_IMMEDIATE_TYPES
template <typename... Args>
using callback = callback_erasure_t<Args...>;
#else
template <typename... Args>
class callback;

template <typename T>
struct is_callback : std::false_type {};
template <typename... Args>
struct is_callback<callback<Args...>> : std::true_type {};

template <typename... Args>
class callback : public callback_erasure_t<Args...> {
public:
  using erasure_t = callback_erasure_t<Args...>;
  erasure_t erasure_;

  callback() = default;
  ~callback() = default;
  callback(callback const&) = delete;
  callback(callback&&) = default;
  callback& operator=(callback const&) = delete;
  callback& operator=(callback&&) = default;

  template <
      typename T,
      std::enable_if_t<std::is_convertible<T, erasure_t>::value>* = nullptr,
      std::enable_if_t<!is_callback<traits::unrefcv_t<T>>::value>* = nullptr>
  /* implicit */ callback(T&& callable) : erasure_(std::forward<T>(callable)) {
  }

  template <
      typename T,
      std::enable_if_t<std::is_assignable<erasure_t, T>::value>* = nullptr,
      std::enable_if_t<!is_callback<traits::unrefcv_t<T>>::value>* = nullptr>
  callback& operator=(T&& callable) {
    erasure_ = std::forward<T>(callable);
    return *this;
  }

  void operator()(Args... args) && noexcept {
    std::move(erasure_)(std::move(args)...);
  }

  void operator()(exception_arg_t exception_arg, exception_t exception) &&
      noexcept {
    std::move(erasure_)(exception_arg, std::move(exception));
  }

  explicit operator bool() const noexcept {
    return bool(erasure_);
  }
};
#endif

using work_erasure_t =
    fu2::function_base<true, false, fu2::capacity_fixed<32UL>, true, false,
                       void()&&, void(exception_arg_t, exception_t) &&>;

#ifdef CONTINUABLE_HAS_IMMEDIATE_TYPES
using work = work_erasure_t;
#else
class work;

template <typename T>
struct is_work : std::false_type {};
template <>
struct is_work<work> : std::true_type {};

class work {
  using erasure_t = work_erasure_t;
  erasure_t erasure_;

public:
  work() = default;
  ~work() = default;
  work(work const&) = delete;
  work(work&&) = default;
  work& operator=(work const&) = delete;
  work& operator=(work&&) = default;

  template <
      typename T,
      std::enable_if_t<std::is_convertible<T, erasure_t>::value>* = nullptr,
      std::enable_if_t<!is_work<traits::unrefcv_t<T>>::value>* = nullptr>
  /* implicit */ work(T&& callable) : erasure_(std::forward<T>(callable)) {
  }

  template <
      typename T,
      std::enable_if_t<std::is_assignable<erasure_t, T>::value>* = nullptr,
      std::enable_if_t<!is_work<traits::unrefcv_t<T>>::value>* = nullptr>
  work& operator=(T&& callable) {
    erasure_ = std::forward<T>(callable);
    return *this;
  }

  void operator()() && noexcept {
    std::move(erasure_)();
  }

  void operator()(exception_arg_t, exception_t exception) && noexcept {
    std::move(erasure_)(exception_arg_t{}, std::move(exception));
  }

  explicit operator bool() const noexcept {
    return bool(erasure_);
  }
};
#endif

template <typename... Args>
struct continuation_capacity {
  using type = union {
    void* pointer_;
    base::ready_continuation<Args...> continuation_;
  };

  static constexpr std::size_t capacity = sizeof(type);
  static constexpr std::size_t alignment = alignof(type);
};

template <typename... Args>
using continuation_erasure_t = fu2::function_base<
    true, false, continuation_capacity<Args...>, true, false,
    void(promise_base<callback<Args...>, signature_arg_t<Args...>>),
    bool(is_ready_arg_t) const, result<Args...>(unpack_arg_t)>;

#ifdef CONTINUABLE_HAS_IMMEDIATE_TYPES
template <typename... Args>
using continuation = continuation_erasure_t<Args...>;
#else
template <typename... Args>
class continuation;

template <typename T>
struct is_continuation : std::false_type {};
template <typename... Args>
struct is_continuation<continuation<Args...>> : std::true_type {};

template <typename... Args>
class continuation {
  using erasure_t = continuation_erasure_t<Args...>;
  erasure_t erasure_;

public:
  continuation() = default;
  ~continuation() = default;
  continuation(continuation const&) = delete;
  continuation(continuation&&) = default;
  continuation& operator=(continuation const&) = delete;
  continuation& operator=(continuation&&) = default;

  template <
      typename T,
      std::enable_if_t<std::is_convertible<T, erasure_t>::value>* = nullptr,
      std::enable_if_t<!is_continuation<traits::unrefcv_t<T>>::value>* =
          nullptr>
  /* implicit */ continuation(T&& callable)
      : erasure_(std::forward<T>(callable)) {
  }

  template <
      typename T,
      std::enable_if_t<std::is_assignable<erasure_t, T>::value>* = nullptr,
      std::enable_if_t<!is_continuation<traits::unrefcv_t<T>>::value>* =
          nullptr>
  continuation& operator=(T&& callable) {
    erasure_ = std::forward<T>(callable);
    return *this;
  }

  void operator()(promise_base<callback<Args...>, //
                               signature_arg_t<Args...>>
                      promise) {
    erasure_(std::move(promise));
  }

  bool operator()(is_ready_arg_t is_ready_arg) const {
    return erasure_(is_ready_arg);
  }

  result<Args...> operator()(unpack_arg_t query_arg) {
    return erasure_(query_arg);
  }
};
#endif
} // namespace erasure
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ERASURE_HPP_INCLUDED
