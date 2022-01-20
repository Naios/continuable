
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

#ifndef CONTINUABLE_RESULT_HPP_INCLUDED
#define CONTINUABLE_RESULT_HPP_INCLUDED

#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/detail/utility/result-trait.hpp>
#include <continuable/detail/utility/result-variant.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

namespace cti {
/// \defgroup Result Result
/// provides the \ref result class and corresponding utility functions to work
/// with the result of an asynchronous operation which can possibly yield:
/// - *no result*: If the operation didn't finish
/// - *a value*: If the operation finished successfully
/// - *an exception*: If the operation finished with an exception
///                   or was cancelled.
/// \{

/// A tag which represents present void values in result.
///
/// \since 4.0.0
using void_arg_t = detail::void_arg_t;

/// A class which is convertible to any \ref result and that definitely holds no
/// value so the real result gets invalidated when this object is passed to it.
///
/// \since 4.0.0
///
struct empty_result {};

/// A class which is convertible to any \ref result and that definitely holds
/// a default constructed exception which signals the cancellation of the
/// asynchronous control flow.
///
/// \since 4.0.0
///
struct cancellation_result {};

/// A class which is convertible to any result and that holds
/// an exception which is then passed to the converted result object.
///
/// \since 4.0.0
///
class exceptional_result {
  exception_t exception_;

public:
  exceptional_result() = delete;
  exceptional_result(exceptional_result const&) = default;
  exceptional_result(exceptional_result&&) = default;
  exceptional_result& operator=(exceptional_result const&) = default;
  exceptional_result& operator=(exceptional_result&&) = default;
  ~exceptional_result() = default;

  explicit exceptional_result(exception_t exception)
    // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
    : exception_(std::move(exception)) {}

  exceptional_result& operator=(exception_t exception) {
    // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
    exception_ = std::move(exception);
    return *this;
  }

  /// Sets an exception
  void set_exception(exception_t exception) {
    // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
    exception_ = std::move(exception);
  }

  /// Returns the contained exception
  exception_t& get_exception() & noexcept {
    return exception_;
  }
  /// \copydoc get_exception
  exception_t const& get_exception() const& noexcept {
    return exception_;
  }
  /// \copydoc get_exception
  exception_t&& get_exception() && noexcept {
    return std::move(exception_);
  }
};

/// The result class can carry the three kinds of results an asynchronous
/// operation possibly can return, it's implemented in a variant like
/// data structure which is also specialized to hold arbitrary arguments.
///
/// The result can be in the following three states:
/// - *no result*: If the operation didn't finish
/// - *a value*: If the operation finished successfully
/// - *an exception*: If the operation finished with an exception
///                   or was cancelled.
///
/// The interface of the result object is similar to the one proposed in
/// the `std::expected` proposal:
/// ```cpp
/// result<std::string> result = make_result("Hello World!");
/// bool(result);
/// result.is_value();
/// result.is_exception();
/// *result; // Same as result.get_value()
/// result.get_value();
/// result.get_exception();
/// ```
///
/// \since 4.0.0
///
template <typename... T>
class result {
  using trait_t = detail::result_trait<T...>;

  template <typename... Args>
  explicit result(detail::init_result_arg_t arg, Args&&... values)
    : variant_(arg, trait_t::wrap(std::forward<Args>(values)...)) {}
  explicit result(detail::init_exception_arg_t arg, exception_t exception)
    : variant_(arg, std::move(exception)) {}

public:
  using value_t = typename trait_t::value_t;
  using value_placeholder_t = typename trait_t::surrogate_t;

  template <typename FirstArg, typename... Args>
  explicit result(FirstArg&& first, Args&&... values)
    : variant_(detail::init_result_arg_t{},
               trait_t::wrap(std::forward<FirstArg>(first),
                             std::forward<Args>(values)...)) {}

  result() = default;
  result(result const&) = delete;
  result(result&&) = default;
  result& operator=(result const&) = delete;
  result& operator=(result&&) = default;
  ~result() = default;

  explicit result(exception_t exception)
    : variant_(detail::init_exception_arg_t{}, std::move(exception)) {}
  /* implicit */ result(empty_result) {}
  /* implicit */ result(exceptional_result exceptional_result)
    : variant_(detail::init_exception_arg_t{},
               std::move(exceptional_result.get_exception())) {}
  /* implicit */ result(cancellation_result)
    : variant_(detail::init_exception_arg_t{}, exception_t{}) {}

  result& operator=(empty_result) {
    variant_.set_empty();
    return *this;
  }
  result& operator=(value_placeholder_t value) {
    variant_.set_value(std::move(value));
    return *this;
  }
  result& operator=(exceptional_result exception) {
    variant_.set_exception(std::move(exception.get_exception()));
    return *this;
  }
  result& operator=(cancellation_result) {
    variant_.set_exception({});
    return *this;
  }

  /// Set the result to an empty state
  void set_empty() {
    variant_.set_empty();
  }
  /// Set the result to a the state which holds the corresponding value
  void set_value(T... values) {
    variant_.set_value(trait_t::wrap(std::move(values)...));
  }
  /// Set the result into a state which holds the corresponding exception
  void set_exception(exception_t exception) {
    variant_.set_exception(std::move(exception));
  }
  /// Set the result into a state which holds the cancellation token
  void set_canceled() {
    variant_.set_exception(exception_t{});
  }

  /// Returns true if the state of the result is empty
  bool is_empty() const noexcept {
    return variant_.is_empty();
  }
  /// Returns true if the state of the result holds the result
  bool is_value() const noexcept {
    return variant_.is_value();
  }
  /// Returns true if the state of the result holds a present exception
  bool is_exception() const noexcept {
    return variant_.is_exception();
  }

  /// \copydoc is_value
  explicit constexpr operator bool() const noexcept {
    return is_value();
  }

  /// Returns the values of the result, if the result doesn't hold the value
  /// the behaviour is undefined but will assert in debug mode.
  decltype(auto) get_value() & noexcept {
    return trait_t::unwrap(variant_.get_value());
  }
  ///\copydoc get_value
  decltype(auto) get_value() const& noexcept {
    return trait_t::unwrap(variant_.get_value());
  }
  ///\copydoc get_value
  decltype(auto) get_value() && noexcept {
    return trait_t::unwrap(std::move(variant_.get_value()));
  }

  ///\copydoc get_value
  decltype(auto) operator*() & noexcept {
    return get_value();
  }
  ///\copydoc get_value
  decltype(auto) operator*() const& noexcept {
    return get_value();
  }
  ///\copydoc get_value
  decltype(auto) operator*() && noexcept {
    return std::move(variant_.get_value());
  }

  /// Returns the exception of the result, if the result doesn't hold an
  /// exception the behaviour is undefined but will assert in debug mode.
  exception_t& get_exception() & noexcept {
    return variant_.get_exception();
  }
  /// \copydoc get_exception
  exception_t const& get_exception() const& noexcept {
    return variant_.get_exception();
  }
  /// \copydoc get_exception
  exception_t&& get_exception() && noexcept {
    return std::move(variant_.get_exception());
  }

  /// Creates a present result from the given values
  static result from(T... values) {
    return result{detail::init_result_arg_t{}, std::move(values)...};
  }
  /// Creates a present result from the given exception
  static result from(exception_arg_t, exception_t exception) {
    return result{detail::init_exception_arg_t{}, std::move(exception)};
  }

  /// Creates an empty result
  static result empty() {
    return result{empty_result{}};
  }

private:
  detail::result_variant<value_placeholder_t> variant_;
};

/// Returns the value at position I of the given result
template <std::size_t I, typename... T>
decltype(auto) get(result<T...>& result) {
  return detail::result_trait<T...>::template get<I>(result);
}
/// \copydoc get
template <std::size_t I, typename... T>
decltype(auto) get(result<T...> const& result) {
  return detail::result_trait<T...>::template get<I>(result);
}
/// \copydoc get
template <std::size_t I, typename... T>
decltype(auto) get(result<T...>&& result) {
  return detail::result_trait<T...>::template get<I>(std::move(result));
}

/// Creates a present result from the given values.
///
/// This could be used to pass the result of the next handler to the same
/// asynchronous path it came from as shown below:
/// ```cpp
/// make_ready_continuable().next([&](auto&&... args) {
///   result<> captured = make_result(std::forward<decltype(args)>(args)...);
///   return shutdown().then([captured = std::move(captured)]() mutable {
///     return std::move(captured);
///   });
/// });
/// ```
///
/// \since 4.0.0
template <typename... T,
          typename Result = result<detail::traits::unrefcv_t<T>...>>
Result make_result(T&&... values) {
  return Result::from(std::forward<T>(values)...);
}

/// Creates an exceptional_result from the given exception.
///
/// \copydetails make_result
///
/// \since 4.0.0
inline exceptional_result make_result(exception_arg_t, exception_t exception) {
  // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
  return exceptional_result{std::move(exception)};
}
/// \}
} // namespace cti

namespace std {
// The GCC standard library defines tuple_size as class and struct which
// triggers a warning here.
#if defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmismatched-tags"
#endif
template <typename... Args>
struct tuple_size<cti::result<Args...>>
  : std::integral_constant<size_t, sizeof...(Args)> {};

template <std::size_t I, typename... Args>
struct tuple_element<I, cti::result<Args...>>
  : tuple_element<I, tuple<Args...>> {};
#if defined(__clang__)
#  pragma GCC diagnostic pop
#endif
} // namespace std

#endif // CONTINUABLE_RESULT_HPP_INCLUDED
