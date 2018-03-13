
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

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

#ifndef CONTINUABLE_BASE_HPP_INCLUDED
#define CONTINUABLE_BASE_HPP_INCLUDED

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <continuable/detail/base.hpp>
#include <continuable/detail/connection-all.hpp>
#include <continuable/detail/connection-any.hpp>
#include <continuable/detail/connection-seq.hpp>
#include <continuable/detail/connection.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
#include <continuable/detail/awaiting.hpp>
#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE

namespace cti {
/// \defgroup Base Base
/// provides classes and functions to create continuable_base objects.
/// \{

/// Represents a tag which can be placed first in a signature
/// in order to overload callables with the asynchronous result
/// as well as an error.
///
/// See the example below:
/// ```cpp
/// struct my_callable {
///   void operator() (std::string result) {
///     // ...
///   }
///   void operator() (cti::dispatch_error_tag, cti::error_type) {
///     // ...
///   }
/// };
///
/// // Will receive errors and results
/// continuable.next(my_callable{});
/// ```
///
/// \note see continuable::next for details.
///
/// \since 2.0.0
using dispatch_error_tag = detail::types::dispatch_error_tag;

/// Represents the type that is used as error type
///
/// By default this type deduces to `std::exception_ptr`.
/// If `CONTINUABLE_WITH_NO_EXCEPTIONS` is defined the type
/// will be a `std::error_condition`.
/// A custom error type may be set through
/// defining `CONTINUABLE_WITH_CUSTOM_ERROR_TYPE`.
///
/// \since 2.0.0
using error_type = detail::types::error_type;

/// Deduces to a true_type if the given type is a continuable_base.
///
/// \since 3.0.0
template <typename T>
using is_continuable = detail::base::is_continuable<T>;

/// The main class of the continuable library, it provides the functionality
/// for chaining callbacks and continuations together to a unified hierarchy.
///
/// The most important method is the cti::continuable_base::then() method,
/// which allows to attach a callback to the continuable.
///
/// Use the continuable types defined in `continuable/continuable.hpp`,
/// in order to use this class.
///
/// \tparam Data The internal data which is used to store the current
///         continuation and intermediate lazy connection result.
///
/// \tparam Annotation The internal data used to store the current signature
///         hint or strategy used for combining lazy connections.
///
/// \note Nearly all methods of the cti::continuable_base are required to be
///       called as r-value. This is required because the continuable carries
///       variables which are consumed when the object is transformed as part
///       of a method call.
///
/// \attention The continuable_base objects aren't intended to be stored.
///            If you want to store a continuble_base you should always
///            call the continuable_base::freeze method for disabling the
///            invocation on destruction.
///
/// \since 1.0.0
template <typename Data, typename Annotation>
class continuable_base {

  /// \cond false
  template <typename, typename>
  friend class continuable_base;
  friend struct detail::base::attorney;

  // The continuation type or intermediate result
  Data data_;
  // The transferable state which represents the validity of the object
  detail::util::ownership ownership_;
  /// \endcond

  /// Constructor accepting the data object while erasing the annotation
  explicit continuable_base(Data data, detail::util::ownership ownership)
      : data_(std::move(data)), ownership_(std::move(ownership)) {
  }

public:
  /// Constructor accepting the data object while erasing the annotation
  explicit continuable_base(Data data) : data_(std::move(data)) {
  }

  /// Constructor accepting any object convertible to the data object,
  /// while erasing the annotation
  template <typename OData, std::enable_if_t<std::is_convertible<
                                std::decay_t<OData>, Data>::value>* = nullptr>
  continuable_base(OData&& data) : data_(std::forward<OData>(data)) {
  }

  /// Constructor taking the data of other continuable_base objects
  /// while erasing the hint.
  ///
  /// This constructor makes it possible to replace the internal data object of
  /// the continuable by any object which is useful for type-erasure.
  template <typename OData, typename OAnnotation>
  continuable_base(continuable_base<OData, OAnnotation>&& other)
      : continuable_base(std::move(other).materialize().consume_data()) {
  }

  /// \cond false
  continuable_base(continuable_base&&) = default;
  continuable_base(continuable_base const&) = delete;

  continuable_base& operator=(continuable_base&&) = default;
  continuable_base& operator=(continuable_base const&) = delete;
  /// \endcond

  /// The destructor automatically invokes the continuable_base
  /// if it wasn't consumed yet.
  ///
  /// In order to invoke the continuable early you may call the
  /// continuable_base::done() method.
  ///
  /// The continuable_base::freeze method disables the automatic
  /// invocation on destruction without invalidating the object.
  ///
  /// \since 1.0.0
  ~continuable_base() {
    if (ownership_.is_acquired() && !ownership_.is_frozen()) {
      std::move(*this).done();
    }
    assert((!ownership_.is_acquired() || ownership_.is_frozen()) &&
           "Ownership should be released!");
  }

  /// Main method of the continuable_base to chain the current continuation
  /// with a new callback.
  ///
  /// \param callback The callback which is used to process the current
  ///        asynchronous result on arrival. The callback is required to accept
  ///        the current result at least partially (or nothing of the result).
  /// ```cpp
  /// (http_request("github.com") && http_request("atom.io"))
  ///   .then([](std::string github, std::string atom) {
  ///     // We use the whole result
  ///   });
  ///
  /// (http_request("github.com") && http_request("atom.io"))
  ///   .then([](std::string github) {
  ///     // We only use the result partially
  ///   });
  ///
  /// (http_request("github.com") && http_request("atom.io"))
  ///   .then([] {
  ///     // We discard the result
  ///   });
  /// ```
  ///
  /// \param executor The optional executor which is used to dispatch
  ///        the callback. The executor needs to accept callable objects
  ///        callable through an `operator()` through its operator() itself.
  ///        The executor can be move-only, but it's not required to.
  ///        The default executor which is used when omitting the argument
  ///        dispatches the callback on the current executing thread.
  ///        Consider the example shown below:
  /// ```cpp
  /// auto executor = [](auto&& work) {
  ///   // Dispatch the work here or forward it to an executor of
  ///   // your choice.
  ///   std::forward<decltype(work)>(work)();
  /// };
  ///
  /// http_request("github.com")
  ///   .then([](std::string github) {
  ///     // Do something...
  ///    }, executor);
  /// ```
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          depending on the return value of the callback:
  /// |      Callback returns      |              Resulting type               |
  /// | : ---------------------- : | : --------------------------------------- |
  /// | `void`                     | `continuable_base with <>`                |
  /// | `Arg`                      | `continuable_base with <Arg>`             |
  /// | `std::pair<First, Second>` | `continuable_base with <First, Second>`   |
  /// | `std::tuple<Args...>`      | `continuable_base with <Args...>`         |
  /// | `continuable_base<Arg...>` | `continuable_base with <Args...>`         |
  ///          Which means the result type of the continuable_base is equal to
  ///          the plain types the callback returns (`std::tuple` and
  ///          `std::pair` arguments are unwrapped).
  ///          A single continuable_base as argument is resolved and the result
  ///          type is equal to the resolved continuable_base.
  ///          Consider the following examples:
  /// ```cpp
  /// http_request("github.com")
  ///   .then([](std::string github) { return; })
  ///   .then([] { }); // <void>
  ///
  /// http_request("github.com")
  ///   .then([](std::string github) { return 0; })
  ///   .then([](int a) { }); // <int>
  ///
  /// http_request("github.com")
  ///   .then([](std::string github) { return std::make_pair(1, 2); })
  ///   .then([](int a, int b) { }); // <int, int>
  ///
  /// http_request("github.com")
  ///   .then([](std::string github) { return std::make_tuple(1, 2, 3); })
  ///   .then([](int a, int b, int c) { }); // <int, int, int>
  ///
  /// http_request("github.com")
  ///   .then([](std::string github) { return http_request("atom.io"); })
  ///   .then([](std::string atom) { }); // <std::string>
  /// ```
  ///
  /// \since 1.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto then(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<detail::base::handle_results::yes,
                                            detail::base::handle_errors::no>(
        std::move(*this).materialize(), std::forward<T>(callback),
        std::forward<E>(executor));
  }

  /// Additional overload of the continuable_base::then() method
  /// which is accepting a continuable_base itself.
  ///
  /// \param continuation A continuable_base reflecting the continuation
  ///        which is used to continue the call hierarchy.
  ///        The result of the current continuable is discarded and the given
  ///        continuation is invoked as shown below.
  /// ```cpp
  /// http_request("github.com")
  ///   .then(http_request("atom.io"))
  ///   .then([](std::string atom) {
  ///     // ...
  ///   });
  /// ```
  ///
  /// \returns Returns a continuable_base representing the next asynchronous
  ///          result to continue within the asynchronous call hierarchy.
  ///
  /// \since 1.0.0
  template <typename OData, typename OAnnotation>
  auto then(continuable_base<OData, OAnnotation>&& continuation) && {
    return std::move(*this).then(
        detail::base::wrap_continuation(std::move(continuation).materialize()));
  }

  /// Main method of the continuable_base to catch exceptions and error codes
  /// in case the asynchronous control flow failed and was resolved
  /// through an error code or exception.
  ///
  /// \param callback The callback which is used to process the current
  ///        asynchronous error result on arrival.
  ///        In case the continuable_base is using exceptions,
  ///        the usage is as shown below:
  ///
  /// ```cpp
  /// http_request("github.com")
  ///   .then([](std::string github) { })
  ///   .fail([](std::exception_ptr ptr) {
  ///     // Handle the error here
  ///     try {
  ///       std::rethrow_exception(ptr);
  ///     } catch (std::exception& e) {
  ///       e.what(); // Handle the exception
  ///     }
  ///   });
  /// ```
  ///        In case exceptions are disabled, `std::error_condition` is
  ///        used as error result instead of `std::exception_ptr`.
  /// ```cpp
  /// http_request("github.com")
  ///   .then([](std::string github) { })
  ///   .fail([](std::error_condition error) {
  ///     error.message(); // Handle the error here
  ///   });
  /// ```
  ///
  /// \param executor The optional executor which is used to dispatch
  ///        the callback. See the description in `then` above.
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          depending on the previous result type.
  ///
  ///
  /// \since 2.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto fail(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<detail::base::handle_results::no,
                                            detail::base::handle_errors::plain>(
        std::move(*this).materialize(), std::forward<T>(callback),
        std::forward<E>(executor));
  }

  /// Additional overload of the continuable_base::fail() method
  /// which is accepting a continuable_base itself.
  ///
  /// \param continuation A continuable_base reflecting the continuation
  ///        which is used to continue the call hierarchy on errors.
  ///        The result of the current continuable is discarded and the given
  ///        continuation is invoked as shown below.
  /// ```cpp
  /// http_request("github.com")
  ///   .fail(http_request("atom.io"))
  /// ```
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          depending on the previous result type.
  ///
  /// \since 2.0.0
  template <typename OData, typename OAnnotation>
  auto fail(continuable_base<OData, OAnnotation>&& continuation) && {
    continuation.freeze();
    return std::move(*this).fail([continuation = std::move(continuation)](
        error_type) mutable { std::move(continuation).done(); });
  }

  /// A method which allows to use an overloaded callable for the error
  /// as well as the valid result path.
  ///
  /// \param callback The callback which is used to process the current
  ///        asynchronous result and error on arrival.
  ///
  /// ```cpp
  /// struct my_callable {
  ///   void operator() (std::string result) {
  ///     // ...
  ///   }
  ///   void operator() (cti::dispatch_error_tag, cti::error_type) {
  ///     // ...
  ///   }
  ///
  /// // Will receive errors and results
  /// http_request("github.com")
  ///   .next(my_callable{});
  /// ```
  ///
  /// \param executor The optional executor which is used to dispatch
  ///        the callback. See the description in `then` above.
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          depending on the current result type.
  ///
  /// \since 2.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto next(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<
        detail::base::handle_results::yes,
        detail::base::handle_errors::forward>(std::move(*this).materialize(),
                                              std::forward<T>(callback),
                                              std::forward<E>(executor));
  }

  /// A method which allows to apply this continuable to the given callable.
  ///
  /// \param transform A transform which shall accept this continuable
  ///
  /// \returns Returns the result of the given transform when this
  ///          continuable is passed into it.
  ///
  /// \since 2.0.0
  template <typename T>
  auto apply(T&& transform) && {
    return std::forward<T>(transform)(std::move(*this).materialize());
  }

  /// The pipe operator | is an alias for the continuable::then method.
  ///
  /// \param right The argument on the right-hand side to connect.
  ///
  /// \returns See the corresponding continuable_base::then method for the
  ///          explanation of the return type.
  ///
  /// \since 2.0.0
  template <typename T>
  auto operator|(T&& right) && {
    return std::move(*this).then(std::forward<T>(right));
  }

  /// The pipe operator | is an alias for the continuable::apply method.
  ///
  /// \param transform The transformer which is applied.
  ///
  /// \returns See the corresponding continuable_base::apply method for the
  ///          explanation of the return type.
  ///
  /// \note    You may create your own transformation through
  ///          calling make_transformation.
  ///
  /// \since 3.0.0
  template <typename T>
  auto operator|(detail::types::transform<T> transform) && {
    return std::move(*this).apply(std::move(transform));
  }

  /// Invokes both continuable_base objects parallel and calls the
  /// callback with the result of both continuable_base objects.
  ///
  /// \param right The continuable on the right-hand side to connect.
  ///
  /// \returns Returns a continuable_base with a result type matching
  ///          the result of the left continuable_base combined with the
  ///          right continuable_base.
  ///          The returned continuable_base will be in an intermediate lazy
  ///          state, further calls to its continuable_base::operator &&
  ///          will add other continuable_base objects to the current
  ///          invocation chain.
  /// ```cpp
  /// (http_request("github.com") && http_request("atom.io"))
  ///   .then([](std::string github, std::string atom) {
  ///     // ...
  ///   });
  ///
  /// auto request = http_request("github.com") && http_request("atom.io");
  /// (std::move(request) && http_request("travis-ci.org"))
  ///    // All three requests are invoked in parallel although we added
  ///    // the request to "travis-ci.org" last.
  ///   .then([](std::string github, std::string atom, std::string travis) {
  ///     // ...
  ///   });
  /// ```
  ///
  /// \note The continuable_base objects are invoked all at onve,
  ///       because the `all` strategy tries to resolve
  ///       the continuations as fast as possible.
  ///       Sequential invocation is also supported through the
  ///       continuable_base::operator>> method.
  ///
  /// \since 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator&&(continuable_base<OData, OAnnotation>&& right) && {
    return detail::connection::connect(
        detail::connection::connection_strategy_all_tag{}, std::move(*this),
        std::move(right));
  }

  /// Invokes both continuable_base objects parallel and calls the
  /// callback once with the first result available.
  ///
  /// \param right The continuable on the right-hand side to connect.
  ///              The right continuable is required to have the same
  ///              result as the left connected continuable_base.
  ///
  /// \returns Returns a continuable_base with a result type matching
  ///          the combined result which of all connected
  ///          continuable_base objects.
  ///          The returned continuable_base will be in an intermediate lazy
  ///          state, further calls to its continuable_base::operator ||
  ///          will add other continuable_base objects to the current
  ///          invocation chain.
  /// ```cpp
  /// (http_request("github.com") || http_request("atom.io"))
  ///   .then([](std::string github_or_atom) {
  ///     // ...
  ///   });
  ///
  /// (make_ready_continuable(10, 'A') || make_ready_continuable(29, 'B'))
  ///   .then([](int a, char b) {
  ///     // ...
  ///   });
  /// ```
  ///
  /// \note The continuable_base objects are invoked all at once,
  ///       however, the callback is only called once with
  ///       the first result or exception which becomes available.
  ///
  /// \since 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator||(continuable_base<OData, OAnnotation>&& right) && {
    return detail::connection::connect(
        detail::connection::connection_strategy_any_tag{}, std::move(*this),
        std::move(right));
  }

  /// Invokes both continuable_base objects sequential and calls the
  /// callback with the result of both continuable_base objects.
  ///
  /// \param right The continuable on the right-hand side to connect.
  ///
  /// \returns Returns a continuable_base with a result type matching
  ///          the result of the left continuable_base combined with the
  ///          right continuable_base.
  /// ```cpp
  /// (http_request("github.com") >> http_request("atom.io"))
  ///   .then([](std::string github, std::string atom) {
  ///     // The callback is called with the result of both requests,
  ///     // however, the request to atom was started after the request
  ///     // to github was finished.
  ///   });
  /// ```
  ///
  /// \note The continuable_base objects are invoked sequential one after
  ///       the previous one was finished. Parallel invocation is also
  ///       supported through the continuable_base::operator && method.
  ///
  /// \since 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator>>(continuable_base<OData, OAnnotation>&& right) && {
    return detail::connection::seq::sequential_connect(std::move(*this),
                                                       std::move(right));
  }

  /// Invokes the continuation chain manually even before the
  /// cti::continuable_base is destructed. This will release the object.
  ///
  /// \see continuable_base::~continuable_base() for further details about
  ///      the continuation invocation on destruction.
  ///
  /// \attention This method will trigger an assertion if the
  ///            continuable_base was released already.
  ///
  /// \since 1.0.0
  void done() && {
    detail::base::finalize_continuation(std::move(*this));
  }

  /// Predicate to check whether the cti::continuable_base is frozen or not.
  ///
  /// \returns Returns true when the continuable_base is frozen.
  ///
  /// \see continuable_base::freeze for further details.
  ///
  /// \attention This method will trigger an assertion if the
  ///            continuable_base was released already.
  ///
  /// \since 1.0.0
  bool is_frozen() const noexcept {
    assert_acquired();
    return ownership_.is_frozen();
  }

  /// Prevents the automatic invocation of the continuation chain
  /// which happens on destruction of the continuable_base.
  /// You may still invoke the chain through the continuable_base::done method.
  ///
  /// This is useful for storing a continuable_base inside a continuation
  /// chain while storing it for further usage.
  ///
  /// \param enabled Indicates whether the freeze is enabled or disabled.
  ///
  /// \see continuable_base::~continuable_base() for further details about
  ///      the continuation invocation on destruction.
  ///
  /// \attention This method will trigger an assertion if the
  ///            continuable_base was released already.
  ///
  /// \since 1.0.0
  continuable_base& freeze(bool enabled = true) & noexcept {
    ownership_.freeze(enabled);
    return *this;
  }

  /// \copydoc continuable_base::freeze
  continuable_base&& freeze(bool enabled = true) && noexcept {
    ownership_.freeze(enabled);
    return std::move(*this);
  }

  /// \cond false
#ifdef CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
  /// \endcond
  /// Implements the operator for awaiting on continuables using `co_await`.
  ///
  /// The operator is only enabled if `CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE`
  /// is defined and the toolchain supports experimental coroutines.
  ///
  /// The return type of the `co_await` expression is specified as following:
  /// |          Continuation type        |          co_await returns          |
  /// | : ------------------------------- | : -------------------------------- |
  /// | `continuable_base with <>`        | `void`                             |
  /// | `continuable_base with <Arg>`     | `Arg`                              |
  /// | `continuable_base with <Args...>` | `std::tuple<Args...>`              |
  ///
  /// When exceptions are used the usage is as intuitive as shown below:
  /// ```cpp
  /// // Handling the exception isn't required and
  /// // the try catch clause may be omitted.
  /// try {
  ///   std::string response = co_await http_request("github.com");
  /// } (std::exception& e) {
  ///   e.what();
  /// }
  /// ```
  ///
  /// In case the library is configured to use error codes or a custom
  /// error type the return type of the co_await expression is changed.
  /// The result is returned through an internal proxy object which may
  /// be queried for the error object.
  /// |          Continuation type        |          co_await returns          |
  /// | : ------------------------------- | : -------------------------------- |
  /// | `continuable_base with <>`        | `unspecified<void>`                |
  /// | `continuable_base with <Arg>`     | `unspecified<Arg>`                 |
  /// | `continuable_base with <Args...>` | `unspecified<std::tuple<Args...>>` |
  /// The interface of the proxy object is similar to the one proposed in
  /// the `std::expected` proposal:
  /// ```cpp
  /// if (auto&& result = co_await http_request("github.com")) {
  ///   auto value = *result;
  /// } else {
  ///   cti::error_type error = result.get_exception();
  /// }
  ///
  /// auto result = co_await http_request("github.com");
  /// bool(result);
  /// result.is_value();
  /// result.is_exception();
  /// *result; // Same as result.get_value()
  /// result.get_value();
  /// result.get_exception();
  /// ```
  ///
  /// \attention Note that it isn't possible as of now to use a continuable
  ///            as return type from coroutines as depicted below:
  /// ```cpp
  /// cti::continuable<int> do_sth() {
  ///   co_await http_request("github.com");
  ///   // ...
  ///   co_return 0;
  /// }
  /// ```
  ///            Propably this will be added in a future version of the library.
  ///
  /// \since     2.0.0
  auto operator co_await() && {
    return detail::awaiting::create_awaiter(std::move(*this).materialize());
  }
  /// \cond false
#endif // CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
  /// \endcond

private:
  void release() noexcept {
    ownership_.release();
  }

  auto materialize() && {
    return detail::connection::materializer<continuable_base>::apply(
        std::move(*this));
  }

  Data&& consume_data() && {
    assert_acquired();
    release();
    return std::move(data_);
  }

  void assert_acquired() const {
    assert(ownership_.is_acquired() && "Tried to use a released continuable!");
  }
};

/// Creates a continuable_base from a promise/callback taking function.
///
/// \tparam Args The types (signature hint) the given promise is resolved with.
/// * **Some arguments** indicate the types the promise will be invoked with.
/// ```cpp
/// auto ct = cti::make_continuable<int, std::string>([](auto&& promise) {
///   promise.set_value(200, "<html>...</html>");
/// });
/// ```
/// * `void` **as argument** indicates that the promise will be invoked
///   with no arguments:
/// ```cpp
/// auto ct = cti::make_continuable<void>([](auto&& promise) {
///   promise.set_value();
/// });
/// ```
/// * **No arguments** Since version 3.0.0 make_continuable always requires
///   to be given valid arguments!
///   You should always give the type hint a callback is called with because
///   it's required for intermediate actions like connecting continuables.
///   You may omit the signature hint if you are erasing the type of
///   the continuable right after creation.
/// ```cpp
/// // This won't work because the arguments are missing:
/// auto ct = cti::make_continuable([](auto&& promise) {
///   promise.set_value(0.f, 'c');
/// });
///
/// // However, you are allowed to do this:
/// cti::continuable<float, char> ct = [](auto&& promise) {
///   promise.set_value(callback)(0.f, 'c');
/// };
/// ```
///
/// \param continuation The continuation the continuable is created from.
/// The continuation must be a callable type accepting a callback parameter
/// which represents the object invokable with the asynchronous result of this
/// continuable.
/// ```cpp
/// auto ct = cti::make_continuable<std::string>([](auto&& promise) {
///   promise.set_value("result");
/// });
/// ```
/// The callback may be stored or moved.
/// In some cases the callback may be copied if supported by the underlying
/// callback chain, in order to invoke the call chain multiple times.
/// It's recommended to accept any callback instead of erasing it.
/// ```cpp
/// // Good practice:
/// auto ct = cti::make_continuable<std::string>([](auto&& promise) {
///   promise.set_value("result");
/// });
///
/// // Good practice using a callable object:
/// struct Continuation {
///   template<typename T>
///   void operator() (T&& continuation) && {
///     // ...
///   }
/// }
///
/// auto ct = cti::make_continuable<std::string>(Continuation{});
///
/// // Bad practice (because of unnecessary type erasure):
/// auto ct = cti::make_continuable<std::string>(
///   [](cti::promise<std::string> promise) {
///     promise.set_value("result");
///   });
/// ```
///
/// \returns A continuable_base with unspecified template parameters which
///          wraps the given continuation.
///          In order to convert the continuable_base to a known type
///          you need to apply type erasure through the
///          \link cti::continuable continuable\endlink or
///          \link cti::promise promise\endlink facilities.
///
/// \note You should always turn the callback/promise into a r-value if possible
///       (`std::move` or `std::forward`) for qualifier correct invokation.
///       Additionally it's important to know that all continuable promises
///       are callbacks and just expose their call operator nicely through
///       \link cti::promise_base::set_value set_value \endlink and
///       \link cti::promise_base::set_exception set_exception \endlink.
///
/// \since 1.0.0
template <typename... Args, typename Continuation>
constexpr auto make_continuable(Continuation&& continuation) {
  static_assert(sizeof...(Args) > 0,
                "Since version 3.0.0 make_continuable requires an exact "
                "signature! If you did intend to create a void continuable "
                "use make_continuable<void>(...). Continuables with an exact "
                "signature may be created through make_continuable<Args...>.");

  return detail::base::attorney::create(
      std::forward<Continuation>(continuation),
      detail::hints::extract(detail::traits::identity<Args...>{}),
      detail::util::ownership{});
}

/// Returns a continuable_base with no result which instantly resolves
/// the promise with no values.
///
/// \attention Usually using this function isn't needed at all since
///            the continuable library is capable of working with
///            plain values in most cases.
///            Try not to use it since it causes unneccessary recursive
///            function calls.
///
/// \since     3.0.0
template <typename... Args>
constexpr auto make_ready_continuable() {
  return make_continuable<void>([](auto&& promise) {
    std::forward<decltype(promise)>(promise).set_value();
  });
}

/// Returns a continuable_base with one result value which instantly resolves
/// the promise with the given value.
///
/// \copydetails make_ready_continuable()
template <typename Result>
constexpr auto make_ready_continuable(Result&& result) {
  return make_continuable<std::decay_t<Result>>( // ...
      [result = std::forward<Result>(result)](auto&& promise) mutable {
        std::forward<decltype(promise)>(promise).set_value(std::move(result));
      });
}

/// Returns a continuable_base with multiple result values which instantly
/// resolves the promise with the given values.
///
/// \copydetails make_ready_continuable()
template <typename FirstResult, typename SecondResult, typename... Rest>
constexpr auto make_ready_continuable(FirstResult&& first_result,
                                      SecondResult&& second_result,
                                      Rest&&... rest) {
  return make_continuable<std::decay_t<FirstResult>, std::decay_t<SecondResult>,
                          std::decay_t<Rest>...>( // ...
      [result = std::make_tuple(std::forward<FirstResult>(first_result),
                                std::forward<SecondResult>(second_result),
                                std::forward<Rest>(rest)...)](
          auto&& promise) mutable {
        detail::traits::unpack(std::forward<decltype(promise)>(promise),
                               result);
      });
}

/// Returns a continuable_base with the parameterized result which instantly
/// resolves the promise with the given error type.
///
/// See an example below:
/// ```cpp
/// std::logic_error exception("Some issue!");
/// auto ptr = std::make_exception_ptr(exception);
/// auto ct = cti::make_exceptional_continuable<int>(ptr);
/// ```
///
/// \tparam Signature The fake signature of the returned continuable.
///
/// \since            3.0.0
template <typename... Signature, typename Exception>
constexpr auto make_exceptional_continuable(Exception&& exception) {
  static_assert(sizeof...(Signature) > 0,
                "Requires at least one type for the fake signature!");

  return make_continuable<Signature...>( // ...
      [exception = std::forward<Exception>(exception)](auto&& promise) mutable {
        std::forward<decltype(promise)>(promise).set_exception(
            std::move(exception));
      });
}
/// \}
} // namespace cti

#endif // CONTINUABLE_BASE_HPP_INCLUDED
