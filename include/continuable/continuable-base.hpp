
/**

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2017 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_BASE_HPP_INCLUDED__
#define CONTINUABLE_BASE_HPP_INCLUDED__

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <continuable/detail/api.hpp>
#include <continuable/detail/base.hpp>
#include <continuable/detail/composition.hpp>
#include <continuable/detail/traits.hpp>
#include <continuable/detail/types.hpp>
#include <continuable/detail/util.hpp>

namespace cti {
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
  continuable_base(continuable_base const&) = default;

  continuable_base& operator=(continuable_base&&) = default;
  continuable_base& operator=(continuable_base const&) = default;
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
  /// \since version 1.0.0
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
  ///        the callback. The executor needs to accept functional objects
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
  /// \since version 1.0.0
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
  /// \since version 1.0.0
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
  ///          depending on the current result type.
  ///
  ///
  /// \since version 2.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto fail(T&& callback,
            E&& executor = detail::types::this_thread_executor_tag{}) && {
    return detail::base::chain_continuation<detail::base::handle_results::no,
                                            detail::base::handle_errors::plain>(
        std::move(*this).materialize(), std::forward<T>(callback),
        std::forward<E>(executor));
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
  ///   .flow(my_callable{});
  /// ```
  ///
  /// \param executor The optional executor which is used to dispatch
  ///        the callback. See the description in `then` above.
  ///
  /// \returns Returns a continuable_base with an asynchronous return type
  ///          depending on the current result type.
  ///
  /// \since version 2.0.0
  template <typename T, typename E = detail::types::this_thread_executor_tag>
  auto flow(T&& callback,
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
  /// \since version 2.0.0
  template <typename T>
  auto apply(T&& transform) && {
    return std::forward<T>(transform)(std::move(*this).materialize());
  }

  /// The pipe operator | is an alias for the continuable::then method.
  ///
  /// \param right The argument on the right-hand side to connect.
  ///
  /// \returns See the corresponding continuable::then method for the
  ///          explanation of the return type.
  ///
  /// \since version 2.0.0
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
  /// \note The continuable_base objects are invoked parallel on the
  ///       current thread, because the `all` strategy tries to resolve
  ///       the continuations as fast as possible.
  ///       Sequential invocation is also supported through the
  ///       continuable_base::operator>> method.
  ///
  /// \since version 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator&&(continuable_base<OData, OAnnotation>&& right) && {
    return detail::composition::connect(detail::composition::strategy_all_tag{},
                                        std::move(*this), std::move(right));
  }

  /// Invokes both continuable_base objects parallel and calls the
  /// callback once with the first result available.
  ///
  /// \param right The continuable on the right-hand side to connect.
  ///              The right continuable is required to have a compatible
  ///              result to the left connected continuable_base,
  ///              such that `std::common_type_t<Left, Right>` deduces to
  ///              a variable for every type in the result of the left and
  ///              the right continuable_base.
  ///
  /// \returns Returns a continuable_base with a result type matching
  ///          the combined result which of all connected
  ///          continuable_base objects.
  ///          The combined result is evaluated through the `std::common_type`
  ///          trait which returns the type all types can be converted to.
  ///          The returned continuable_base will be in an intermediate lazy
  ///          state, further calls to its continuable_base::operator &&
  ///          will add other continuable_base objects to the current
  ///          invocation chain.
  /// ```cpp
  /// (http_request("github.com") || http_request("atom.io"))
  ///   .then([](std::string github_or_atom) {
  ///     // ...
  ///   });
  ///
  /// (supply(10, 'T') || supply(10.f, 'T'))
  ///   .then([](int a, char b) {
  ///     // ...
  ///   });
  /// ```
  ///
  /// \note The continuable_base objects are invoked parallel on the
  ///       current thread, however, the callback is only called once with
  ///       the first result which becomes available.
  ///
  /// \since version 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator||(continuable_base<OData, OAnnotation>&& right) && {
    return detail::composition::connect(detail::composition::strategy_any_tag{},
                                        std::move(*this), std::move(right));
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
  /// \note The continuable_base objects are invoked sequential on the
  ///       current thread. Parallel invocation is also supported through the
  ///       continuable_base::operator&& method.
  ///
  /// \since version 1.0.0
  template <typename OData, typename OAnnotation>
  auto operator>>(continuable_base<OData, OAnnotation>&& right) && {
    return detail::composition::sequential_connect(std::move(*this),
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
  /// \since version 1.0.0
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
  /// \since version 1.0.0
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
  /// \since version 1.0.0
  continuable_base& freeze(bool enabled = true) & noexcept {
    ownership_.freeze(enabled);
    return *this;
  }

  /// \copydoc continuable_base::freeze
  continuable_base&& freeze(bool enabled = true) && noexcept {
    ownership_.freeze(enabled);
    return std::move(*this);
  }

private:
  void release() noexcept {
    ownership_.release();
  }

  auto materialize() &&
      noexcept(std::is_nothrow_move_constructible<Data>::value) {
    assert_acquired();
    return materializeImpl(std::move(*this));
  }

  template <
      typename OData, typename OAnnotation,
      std::enable_if_t<!detail::composition::is_strategy<OAnnotation>::value>* =
          nullptr>
  static auto
  materializeImpl(continuable_base<OData, OAnnotation>&& continuable) {
    return std::move(continuable);
  }
  template <
      typename OData, typename OAnnotation,
      std::enable_if_t<detail::composition::is_strategy<OAnnotation>::value>* =
          nullptr>
  static auto
  materializeImpl(continuable_base<OData, OAnnotation>&& continuable) {
    return detail::composition::finalize_composition(std::move(continuable));
  }

  Data&& consume_data() && {
    release();
    return std::move(data_);
  }

  void assert_acquired() const {
    assert(ownership_.is_acquired() && "Tried to use a released continuable!");
  }
};

/// Creates a continuable_base from a callback taking function.
///
/// \tparam Args The types (signature hint) the given callback is called with.
/// * **Some arguments** indicate the types the callback will be invoked with.
/// ```cpp
/// auto ct = cti::make_continuable<int, std::string>([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(200, "<html>...</html>");
/// });
/// ```
/// * **void as argument** indicates that the callback will be invoked
///   with no arguments:
/// ```cpp
/// auto ct = cti::make_continuable<void>([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)();
/// });
/// ```
/// * **No arguments** indicate that the types are unknown.
///   You should always give the type hint a callback is called with because
///   it's required for intermediate actions like connecting continuables.
///   You may omit the signature hint if you are erasing the type of
///   the continuable right after creation.
/// ```cpp
/// // Never do this:
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(0.f, 'c');
/// });
///
/// // However, you may do this:
/// continuable<float, char> ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)(0.f, 'c');
/// });
/// ```
///
/// \param continuation The continuation the continuable is created from.
/// The continuation must be a functional type accepting a callback parameter
/// which represents the object invokable with the asynchronous result of this
/// continuable.
/// ```cpp
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)("result");
/// });
/// ```
/// The callback may be stored or moved.
/// In some cases the callback may be copied if supported by the underlying
/// callback chain, in order to invoke the call chain multiple times.
/// It's recommended to accept any callback instead of erasing it.
/// ```cpp
/// // Good practice:
/// auto ct = cti::make_continuable([](auto&& callback) {
///   std::forward<decltype(callback)>(callback)("result");
/// });
///
/// // Good practice using a functional object:
/// struct Continuation {
///   template<typename T>
///   void operator() (T&& continuation) const {
///     // ...
///   }
/// }
///
/// auto ct = cti::make_continuable(Continuation{});
///
/// // Bad practice (because of unnecessary type erasure):
/// auto ct = cti::make_continuable(
///   [](std::function<void(std::string)> callback) {
///     callback("result");
///   });
/// ```
///
/// \returns A continuable_base with unknown template parameters which
///          wraps the given continuation.
///          In order to convert the continuable_base to a known type
///          you need to apply type erasure.
///
/// \note You should always turn the callback into a r-value if possible
///       (`std::move` or `std::forward`) for qualifier correct invokation.
///
/// \since version 1.0.0
template <typename... Args, typename Continuation>
auto make_continuable(Continuation&& continuation) {
  auto hint = detail::composition::annotating::extract(
      detail::traits::identity_of(continuation),
      detail::traits::identity<Args...>{});

  return detail::base::attorney::create(
      std::forward<Continuation>(continuation), hint,
      detail::util::ownership{});
}

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
/// continuable.flow(my_callable{});
/// ```
///
/// \note see continuable::flow for details.
///
/// \since version 2.0.0
using detail::types::dispatch_error_tag;

/// Represents the type that is used as error type
///
/// By default this type deduces to `std::exception_ptr`.
/// If `CONTINUABLE_WITH_NO_EXCEPTIONS` is defined the type
/// will be a `std::error_condition`.
/// A custom error type may be set through
/// defining `CONTINUABLE_WITH_CUSTOM_ERROR_TYPE`.
///
/// \since version 2.0.0
using detail::types::error_type;

/// Connects the given continuables with an *all* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator && for details.
///
/// \since version 1.1.0
template <typename... Continuables>
auto when_all(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return CONTINUABLE_FOLD_EXPRESSION(
      &&, std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with an *any* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator|| for details.
///
/// \since version 1.1.0
template <typename... Continuables>
auto when_any(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return CONTINUABLE_FOLD_EXPRESSION(
      ||, std::forward<Continuables>(continuables)...);
}

/// Connects the given continuables with a *seq* logic.
///
/// \param continuables The continuable_base objects to connect.
///        Requires at least 2 objects to connect.
///
/// \see continuable_base::operator>> for details.
///
/// \since version 1.1.0
template <typename... Continuables>
auto when_seq(Continuables&&... continuables) {
  static_assert(sizeof...(continuables) >= 2,
                "Requires at least 2 continuables!");
  return CONTINUABLE_FOLD_EXPRESSION(
      >>, std::forward<Continuables>(continuables)...);
}
} // namespace cti

#endif // CONTINUABLE_BASE_HPP_INCLUDED__
