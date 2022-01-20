
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

#ifndef CONTINUABLE_BASE_HPP_INCLUDED
#define CONTINUABLE_BASE_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/detail/connection/connection-all.hpp>
#include <continuable/detail/connection/connection-any.hpp>
#include <continuable/detail/connection/connection-seq.hpp>
#include <continuable/detail/connection/connection.hpp>
#include <continuable/detail/core/base.hpp>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/features.hpp>
#include <continuable/detail/utility/traits.hpp>
#include <continuable/detail/utility/util.hpp>

#if defined(CONTINUABLE_HAS_COROUTINE)
#  include <continuable/detail/other/coroutines.hpp>
#endif // defined(CONTINUABLE_HAS_COROUTINE)

namespace cti {
/// \defgroup Base Base
/// provides classes and functions to create continuable_base objects.
/// \{

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
  using ownership = detail::util::ownership;

  using annotation_trait = detail::annotation_trait<Annotation>;

  template <typename, typename>
  friend class continuable_base;
  friend struct detail::base::attorney;

  // The continuation type or intermediate result
  Data data_;
  // The transferable state which represents the validity of the object
  ownership ownership_;
  /// \endcond

  /// Constructor accepting the data object while erasing the annotation
  explicit continuable_base(Data data, ownership ownership)
    : data_(std::move(data))
    , ownership_(std::move(ownership)) {}

public:
  /// Constructor accepting the data object while erasing the annotation
  explicit continuable_base(Data data)
    : data_(std::move(data)) {}

  /// Constructor accepting any object convertible to the data object,
  /// while erasing the annotation
  template <typename OtherData,
            std::enable_if_t<detail::base::can_accept_continuation<
                Data, Annotation,
                detail::traits::unrefcv_t<OtherData>>::value>* = nullptr>
  /* implicit */ continuable_base(OtherData&& data)
    : data_(
          detail::base::proxy_continuable<Annotation,
                                          detail::traits::unrefcv_t<OtherData>>(
              std::forward<OtherData>(data))) {}

  /// Constructor taking the data of other continuable_base objects
  /// while erasing the hint.
  ///
  /// This constructor makes it possible to replace the internal data object of
  /// the continuable by any object which is useful for type-erasure.
  template <typename OData,
            std::enable_if_t<std::is_convertible<
                detail::traits::unrefcv_t<OData>, Data>::value>* = nullptr>
  /* implicit */ continuable_base(continuable_base<OData, Annotation>&& other)
    : data_(std::move(other).consume()) {}

  /// Constructor taking the data of other continuable_base objects
  /// while erasing the hint.
  ///
  /// This constructor makes it possible to replace the internal data object of
  /// the continuable by any object which is useful for type-erasure.
  template <typename OData, typename OAnnotation>
  /* implicit */ continuable_base(continuable_base<OData, OAnnotation>&& other)
    : continuable_base(std::move(other).finish().consume()) {}

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
  /// | `cti::result<Args...>`     | `continuable_base with <Args...>`         |
  /// | `continuable_base<Arg...>` | `continuable_base with <Args...>`         |
  ///          Which means the result type of the continuable_base is equal to
  ///          the plain types the callback returns (`std::tuple` and
  ///          `std::pair` arguments are unwrapped).
  ///          A single continuable_base as argument is resolved and the result
  ///          type is equal to the resolved continuable_base.
  ///          A cti::result can be used to cancel the continuation or to
  ///          transition to the exception handler.
  ///          The special unwrapping of types can be disabled through wrapping
  ///          such objects through a call to cti::make_plain.
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
  ///
  /// http_request("example.com")
  ///   .then([](std::string content) -> result<std::string> {
  ///     return rethrow(std::make_exception_ptr(std::exception{}));
  ///   })
  ///   .fail([] -> result<std::string> {
  ///     return recover("Hello World!");
  ///   })
  ///   .then([](std::string content) -> result<std::string> {
  ///     return cancel();
  ///   })
  /// ```
  ///
  /// \since 1.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto then(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<detail::base::handle_results::yes,
                                            detail::base::handle_errors::no>(
        std::move(*this).finish(), std::forward<T>(callback),
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
        detail::base::wrap_continuation(std::move(continuation).finish()));
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
  ///   .fail([](std::exception_ptr ep) {
  ///     // Check whether the exception_ptr is valid (not default constructed)
  ///     // if bool(ep) == false this means that the operation was cancelled
  ///     // by the user or application (promise.set_canceled() or
  ///     // make_cancelling_continuable()).
  ///     if (ep) {
  ///       // Handle the error here
  ///       try {
  ///         std::rethrow_exception(ep);
  ///       } catch (std::exception& e) {
  ///         e.what(); // Handle the exception
  ///       }
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
  /// \attention The given exception type exception_t can be passed to the
  ///            handler in a default constructed state <br>`bool(e) == false`.
  ///            This always means that the operation was cancelled by the user,
  ///            possibly through:
  ///              - \ref promise_base::set_canceled
  ///              - \ref make_cancelling_continuable
  ///              - \ref result::set_canceled
  ///              - \ref cancel<br>
  ///            In that case the exception can be ignored safely (but it is
  ///            recommended not to proceed, although it is possible to
  ///            recover from the cancellation).
  ///
  /// \since 2.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto fail(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<
        detail::base::handle_results::no, detail::base::handle_errors::forward>(
        std::move(*this).finish(),
        detail::base::strip_exception_arg(std::forward<T>(callback)),
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
    return std::move(*this)                                     //
        .fail([continuation = std::move(continuation).freeze()] //
              (exception_t) mutable {
                std::move(continuation).done(); //
              });
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
  ///   void operator() (cti::exception_arg_t, cti::exception_t) {
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
        detail::base::handle_errors::forward>(std::move(*this).finish(),
                                              std::forward<T>(callback),
                                              std::forward<E>(executor));
  }

  /// Returns a continuable_base which continues its invocation through the
  /// given executor.
  ///
  /// \returns Returns a continuable_base of the same type.
  ///
  /// \since 4.2.0
  template <typename E>
  auto via(E&& executor) && {
    return std::move(*this).next(
        [](auto&&... args) {
          return make_result(std::forward<decltype(args)>(args)...);
        },
        std::forward<E>(executor));
  }

  /// Returns a continuable_base which will have its signature converted
  /// to the given Args.
  ///
  /// A signature can only be converted if it can be partially applied
  /// from the previous one as shown below:
  /// ```cpp
  /// continuable<long> c = make_ready_continuable(0, 1, 2).as<long>();
  /// ```
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          matching the given Args.
  ///
  /// \since 4.0.0
  template <typename... Args>
  auto as() && {
    return std::move(*this).then(detail::base::convert_to<Args...>{});
  }

  /// A method which allows to apply a callable object to this continuable.
  ///
  /// \param transform A callable objects that transforms a continuable
  ///                  to a different object.
  ///
  /// \returns Returns the result of the given transform when this
  ///          continuable is passed into it.
  ///
  /// \since 4.0.0
  template <typename T>
  auto apply(T&& transform) && {
    return std::forward<T>(transform)(std::move(*this).finish());
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
    detail::base::finalize_continuation(std::move(*this).finish());
  }

  /// Materializes the continuation expression template and finishes
  /// the current applied strategy such that the resulting continuable
  /// will always be a concrete type and Continuable::is_concrete holds.
  ///
  /// This can be used in the case where we are chaining continuations lazily
  /// through a strategy, for instance when applying operators for
  /// expressing connections and then want to return a materialized
  /// continuable_base which uses the strategy respectively.
  /// ```cpp
  /// auto do_both() {
  ///   return (wait(10s) || wait_key_pressed(KEY_SPACE)).finish();
  /// }
  ///
  /// // Without a call to finish() this would lead to
  /// // an unintended evaluation strategy:
  /// do_both() || wait(5s);
  /// ```
  ///
  /// \note When using a type erased continuable_base such as
  ///       `continuable<...>` this method doesn't need to be called
  ///       since the continuable_base is materialized automatically
  ///       on conversion.
  ///
  /// \since 4.0.0
  auto finish() && {
    return annotation_trait::finish(std::move(*this));
  }

  /// Returns true when the continuable can provide its result immediately,
  /// and its lazy invocation would be side-effect free.
  ///
  /// \since 4.0.0
  bool is_ready() const noexcept {
    return annotation_trait::is_ready(*this);
  }

  /// Invalidates the continuable and returns its immediate invocation result.
  ///
  /// This method can be used to specialize the asynchronous control flow
  /// based on whether the continuable_base is_ready at every time,
  /// which is true for a continuable created through the following functions:
  ///   - make_ready_continuable
  ///   - make_exceptional_continuable
  ///
  /// \returns   A result<Args...> where Args... represent the current
  ///            asynchronous parameters or the currently stored exception.
  ///
  /// \attention unpack requires that continuable_base::is_ready returned true
  ///            in a previous check, otherwise its behaviour is unspecified.
  ///
  /// \since 4.0.0
  auto unpack() && {
    assert(ownership_.is_acquired());
    assert(is_ready());
    return detail::base::attorney::query(std::move(*this).finish());
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
#if defined(CONTINUABLE_HAS_COROUTINE)
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
  /// exception type the return type of the co_await expression is changed.
  /// The result is returned through a cti::result<...>.
  /// |          Continuation type        |          co_await returns          |
  /// | : ------------------------------- | : -------------------------------- |
  /// | `continuable_base with <>`        | `result<void>`                     |
  /// | `continuable_base with <Arg>`     | `result<Arg>`                      |
  /// | `continuable_base with <Args...>` | `result<Args...>`                  |
  ///
  /// \note  Using continuable_base as return type for coroutines
  ///        is supported. The coroutine is initially stopped and
  ///        resumed when the continuation is requested in order to
  ///        keep the lazy evaluation semantics of the continuable_base.
  /// ```cpp
  /// cti::continuable<> resolve_async_void() {
  ///   co_await http_request("github.com");
  ///   // ...
  ///   co_return;
  /// }
  ///
  /// cti::continuable<int> resolve_async() {
  ///   co_await http_request("github.com");
  ///   // ...
  ///   co_return 0;
  /// }
  /// ```
  /// It's possible to return multiple return values from coroutines
  /// by wrapping those in a tuple like type:
  /// ```cpp
  /// cti::continuable<int, int, int> resolve_async_multiple() {
  ///   co_await http_request("github.com");
  ///   // ...
  ///   co_return std::make_tuple(0, 1, 2);
  /// }
  /// ```
  ///
  /// \since     2.0.0
  auto operator co_await() && {
    return detail::awaiting::create_awaiter(std::move(*this).finish());
  }
  /// \cond false
#endif // defined(CONTINUABLE_HAS_COROUTINE)
  /// \endcond

private:
  void release() noexcept {
    ownership_.release();
  }

  Data&& consume() && {
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

  return detail::base::attorney::create_from(
      std::forward<Continuation>(continuation),
      typename detail::hints::from_args<Args...>::type{},
      detail::util::ownership{});
}

/// Returns a continuable_base with no result which instantly resolves
/// the promise with no values.
///
/// \attention Usually using this function isn't needed at all since
///            the continuable library is capable of working with
///            plain values in most cases.
///            Try not to use it since it causes unnecessary recursive
///            function calls.
///
/// \since     3.0.0
template <typename... Args>
auto make_ready_continuable(Args&&... args) {
  return detail::base::attorney::create_from_raw(
      detail::base::ready_continuation<detail::traits::unrefcv_t<Args>...>(
          result<detail::traits::unrefcv_t<Args>...>::from(
              std::forward<Args>(args)...)),
      detail::identity<detail::traits::unrefcv_t<Args>...>{},
      detail::util::ownership{});
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
/// \tparam Args The fake signature of the returned continuable.
///
/// \since            3.0.0
template <typename... Args, typename Exception>
constexpr auto make_exceptional_continuable(Exception&& exception) {
  static_assert(sizeof...(Args) > 0,
                "Requires at least one type for the fake signature!");

  using hint_t = typename detail::hints::from_args<Args...>::type;
  using ready_continuation_t = typename detail::base::
      ready_continuation_from_hint<hint_t>::type;
  using result_t = typename detail::base::result_from_hint<hint_t>::type;
  return detail::base::attorney::create_from_raw(
      ready_continuation_t(result_t::from(exception_arg_t{},
                                          std::forward<Exception>(exception))),
      hint_t{}, detail::util::ownership{});
}

/// Returns a continuable_base with the parameterized result which never
/// resolves its promise and thus cancels the asynchronous continuation chain
/// through throwing a default constructed exception_t.
///
/// This can be used to cancel an asynchronous continuation chain when
/// returning a continuable_base from a handler where other paths could
/// possibly continue the asynchronous chain. See an example below:
/// ```cpp
/// do_sth().then([weak = this->weak_from_this()]() -> continuable<> {
///   if (auto me = weak.lock()) {
///     return do_sth_more();
///   } else {
///     // Abort the asynchronous continuation chain since the
///     // weakly referenced object expired previously.
///     return make_cancelling_continuable<void>();
///   }
/// });
/// ```
/// The default unhandled exception handler ignores exception types
/// that don't evaluate to true when being converted to a bool.
/// This saves expensive construction of std::exception_ptr or similar types,
/// where only one exception type is used for signaling the cancellation.
///
/// \tparam Signature The fake signature of the returned continuable.
///
/// \since            4.0.0
template <typename... Signature>
auto make_cancelling_continuable() {
  static_assert(sizeof...(Signature) > 0,
                "Requires at least one type for the fake signature!");

  return make_exceptional_continuable<Signature...>(exception_t{});
}

/// Can be used to disable the special meaning for a returned value in
/// asynchronous handler functions.
///
/// Several types have a special meaning when being returned from a callable
/// passed to asynchronous handler functions like:
/// - continuable_base::then
/// - continuable_base::fail
/// - continuable_base::next
///
/// For instance such types are std::tuple, std::pair and cti::result.
///
/// Wrapping such an object through a call to make_plain disables the special
/// meaning for such objects as shown below:
/// ```cpp
/// continuable<result<int, int> c = http_request("example.com")
///   .then([](std::string content) {
///     return make_plain(make_result(0, 1));
///   })
/// ```
///
/// \since 4.0.0
///
template <typename T>
auto make_plain(T&& value) {
  return plain_t<detail::traits::unrefcv_t<T>>(std::forward<T>(value));
}

/// Can be used to recover to from a failure handler,
/// the result handler which comes after will be called with the
/// corresponding result.
///
/// The \ref exceptional_result returned by this function can be returned
/// from any result or failure handler in order to rethrow the exception.
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) {
///     return recover(1, 2);
///   })
///   .fail([](cti::exception_t exception) {
///     return recover(1, 2);
///   })
///   .then([](int a, int b) {
///     // Recovered from the failure
///   })
/// ```
/// A corresponding \ref result is returned by \ref recover
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) -> cti::result<int, int> {
///     return recover(1, 2);
///   })
///   .fail([](cti::exception_t exception) -> cti::result<int, int> {
///     return recover(1, 2);
///   })
///   .then([](int a, int b) -> cti::result<int, int> {
///     // Recovered from the failure
///   })
/// ```
///
/// \since 4.0.0
///
template <typename... Args>
result<detail::traits::unrefcv_t<Args>...> recover(Args&&... args) {
  return make_result(std::forward<Args>(args)...);
}

/// Can be used to rethrow an exception to the asynchronous continuation chain,
/// the failure handler which comes after will be called with the
/// corresponding exception.
///
/// The \ref exceptional_result returned by this function can be returned
/// from any result or failure handler in order to rethrow the exception.
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   })
///   .fail([](cti::exception_t exception) {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   })
///   .next([](auto&&...) {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   });
/// ```
/// The returned \ref exceptional_result is convertible to
/// any \ref result as shown below:
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) -> cti::result<> {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   })
///   .fail([](cti::exception_t exception) -> cti::result<> {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   })
///   .next([](auto&&...) -> cti::result<> {
///     return rethrow(std::make_exception_ptr(std::exception{}));
///   });
/// ```
///
/// \since 4.0.0
///
// NOLINTNEXTLINE(performance-unnecessary-value-param)
inline exceptional_result rethrow(exception_t exception) {
  // NOLINTNEXTLINE(hicpp-move-const-arg, performance-move-const-arg)
  return exceptional_result{std::move(exception)};
}

/// Can be used to cancel an asynchronous continuation chain,
/// the next failure handler which comes after cancel will be called
/// with a default constructed exception_t object.
///
/// The \ref cancellation_result returned by this function can be returned from
/// any result or failure handler in order to cancel the chain.
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) {
///     return cancel();
///   })
///   .fail([](cti::exception_t exception) {
///     return cancel();
///   })
///   .next([](auto&&...) {
///     return cancel();
///   });
/// ```
/// The returned \ref empty_result is convertible to
/// any \ref result as shown below:
/// ```cpp
/// http_request("example.com")
///   .then([](std::string content) -> cti::result<> {
///     return cancel();
///   })
///   .fail([](cti::exception_t exception) -> cti::result<> {
///     return cancel();
///   })
///   .next([](auto&&...) -> cti::result<> {
///     return cancel();
///   });
/// ```
///
/// \since 4.0.0
///
inline cancellation_result cancel() {
  return {};
}

/// Can be used to stop an asynchronous continuation chain,
/// no handler which comes after stop was received won't be called.
///
/// \since 4.0.0
///
inline empty_result stop() {
  return {};
}

/// \}
} // namespace cti

#endif // CONTINUABLE_BASE_HPP_INCLUDED
