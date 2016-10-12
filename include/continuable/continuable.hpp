
/**
 * Copyright 2015-2016 Denis Blank <denis.blank@outlook.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONTINUABLE_HPP_INCLUDED__
#define CONTINUABLE_HPP_INCLUDED__

 /*
#include <tuple>
#include <type_traits>
#include <string>

namespace detail {
  template<typename... T>
  struct Identity { };

  template<typename T, typename... Rest>
  struct IdentityInheritenceWrapper
    : Identity<T>, IdentityInheritenceWrapper<Rest...> { };

  enum class NamedParameterId {
    NAMED_PARAMATER_CALLBACK,
    NAMED_PARAMATER_REJECTOR
  };

  template <NamedParameterId Value, typename T>
  struct NamedParameter
    : std::integral_constant<NamedParameterId, Value>,
      std::common_type<T> { };

  template <NamedParameterId Value, typename Default, typename... Args>
  using GetNamedParameterOrDefault = void;

  template<typename, typename>
  class ContinuableBase;

  template<typename T>
  struct ReturnTypeToContinuableConverter;

  template<typename... Args>
  struct


  template<typename... Args, typename CallbackType>
  class ContinuableBase<Identity<Args...>, CallbackType> {
    CallbackType callback_;

  public:
    explicit ContinuableBase(CallbackType&& callback)
      : callback_(std::move(callback)) { }

    void via() { }

    template<typename C>
    auto then(C&& continuation) {
      // The type the callback will evaluate to
      using EvaluatedTo = decltype(std::declval<C>()(std::declval<Args>()...));


      return EvaluatedTo{ };
    }
  };
} // namespace detail

using namespace detail;

// template<typename... Args>
// using Continuable = detail::ContinuableBase<detail::Identity<Args...>>;

template <typename... Args>
struct Callback {
  void operator() (Args... ) { }
};

template <typename... Args>
auto make_continuable(Args&&...) {
  return Continuable<> { };
}

auto http_request(std::string url) {
  return make_continuable([url](auto& callback) {

    callback("<br>hi<br>");
  });
}

 template<typename Continuation, typename Handler>
auto appendHandlerToContinuation(Continuation&& cont, Handler&& handler) {
  return [cont = std::forward<Continuation>(cont),
          handler = std::forward<Handler>(handler)](auto&& continuation) {
    using T = decltype(continuation);
    return [continuation = std::forward<T>(continuation)](auto&&... arg) {
      continuation(std::forward<decltype(arg)>(arg)...);
    };

    current([continuation = std::forward<T>(continuation)](auto&&... arg) {
      continuation(std::forward<decltype(arg)>(arg)...);
    });
  };
} */

#include <functional>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>
#include <string>
#include <memory>

// Equivalent to C++17's std::void_t which is targets a bug in GCC,
// that prevents correct SFINAE behavior.
// See http://stackoverflow.com/questions/35753920 for details.
template<typename...>
struct deduce_to_void : std::common_type<void> { };

template<typename... T>
using always_void_t = typename deduce_to_void<T...>::type;

struct SelfDispatcher {
  template<typename T>
  void operator() (T&& callable) const {
    std::forward<T>(callable)();
  }
};

template<typename T>
struct Identity { };

template<typename Config>
class ContinuableBase;

static auto createEmptyContinuation() {
  return [](auto&& callback) { callback(); };
}

static auto createEmptyCallback() {
  return [](auto&&...) { };
}

template<typename S, unsigned... I, typename T, typename F>
auto applyTuple(std::integer_sequence<S, I...>, T&& tuple, F&& function) {
  return std::forward<F>(function)(std::get<I>(std::forward<T>(tuple))...);
}

template<unsigned... LeftI, unsigned... RightI,
         typename... Left, typename... Right>
auto tupleMerge(std::integer_sequence<unsigned, LeftI...>,
                std::integer_sequence<unsigned, RightI...>,
                std::tuple<Left...>&& left, std::tuple<Right...>&& right) {
  return std::make_tuple(std::get<LeftI>(std::move(left))...,
                         std::get<RightI>(std::move(right))...);
}

/// Merges the left and the right tuple together in one.
template<typename... Left, typename... Right>
auto tupleMerge(std::tuple<Left...>&& left,
                std::tuple<Right...>&& right) {
  return tupleMerge(std::make_integer_sequence<unsigned, sizeof...(Left)>{},
                    std::make_integer_sequence<unsigned, sizeof...(Right)>{},
                    std::move(left), std::move(right));
}

/// This class is responsible for holding an abstract copy- and
/// move-able ownership that is invalidated when the object
/// is moved to another instance.
class Ownership {
public:
  Ownership() { }
  explicit Ownership(bool isOwning_) : isOwning(isOwning_) { }
  Ownership(Ownership const&) = default;
  Ownership(Ownership&& right) noexcept
    : isOwning(std::exchange(right.isOwning, false)) { };
  Ownership& operator = (Ownership const&) = default;
  Ownership& operator = (Ownership&& right) noexcept {
    isOwning = std::exchange(right.isOwning, false);
    return *this;
  }

  Ownership operator&& (Ownership right) const {
    return Ownership(hasOwnership() && right.hasOwnership());
  }

  bool hasOwnership() const noexcept {
    return isOwning;
  }
  void invalidate() {
    isOwning = false;
  }

private:
  bool isOwning{ true };
};

template<typename Function>
struct undecorate_function;

template<typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(Args...)> {
  /// The return type of the function.
  typedef ReturnType return_type;
  /// The argument types of the function as pack in Identity.
  typedef Identity<Args...> argument_type;
};

/// Mutable function pointers
template<typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(*)(Args...)>
  : undecorate_function<ReturnType(Args...)> { };

/// Const function pointers
template<typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(*const)(Args...)>
  : undecorate_function<ReturnType(Args...)> { };

/// Mutable class method pointers
template<typename ClassType, typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(ClassType::*)(Args...)>
  : undecorate_function<ReturnType(Args...)> { };

/// Const class method pointers
template<typename ClassType, typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(ClassType::*)(Args...) const>
  : undecorate_function<ReturnType(Args...)> { };

/// Mutable volatile class method pointers
template<typename ClassType, typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(ClassType::*)(Args...) volatile>
  : undecorate_function<ReturnType(Args...)> { };

/// Const volatile class method pointers
template<typename ClassType, typename ReturnType, typename... Args>
struct undecorate_function<ReturnType(ClassType::*)(Args...) const volatile>
  : undecorate_function<ReturnType(Args...)> { };

template<typename Function>
using do_undecorate = std::conditional_t<
  std::is_class<Function>::value,
  decltype(&Function::operator()),
  Function>;

template<typename Function, typename = always_void_t<>>
struct is_undecorateable : std::false_type { };

template<typename Function>
struct is_undecorateable<Function, always_void_t<
  typename do_undecorate<Function>::return_type
>> : std::true_type { };

/// Decorates single values
template<typename Value>
struct CallbackResultDecorator {
  template<typename Callback>
  static auto decorate(Callback&& callback) {
    return [callback = std::forward<Callback>(callback)](auto&&... args) {
      Value value = callback(std::forward<decltype(args)>(args)...);
      return [value =  std::move(value)](auto&& callback) mutable {
        callback(std::move(value));
      };
    };
  }
};

/// No decoration is needed for continuables
template<typename Decorator>
struct CallbackResultDecorator<ContinuableBase<Decorator>>{
  template<typename Callback>
  static auto decorate(Callback&& callback) -> std::decay_t<Callback> {
    return std::forward<Callback>(callback);
  }
};

/// Decorates void as return type
template<>
struct CallbackResultDecorator<void> {
  template<typename Callback>
  static auto decorate(Callback&& callback) {
    return [callback = std::forward<Callback>(callback)](auto&&... args) {
      callback(std::forward<decltype(args)>(args)...);
      return createEmptyContinuation();
    };
  }
};

// Decorates tuples as return type
template<typename... Results>
struct CallbackResultDecorator<std::tuple<Results...>> {
  template<typename Callback>
  static auto decorate(Callback&& callback) {
    return [callback = std::forward<Callback>(callback)](auto&&... args) {
      // Receive the tuple from the callback
      auto result = callback(std::forward<decltype(args)>(args)...);
      return [result = std::move(result)] (auto&& next) mutable {
        // Generate a sequence for tag dispatching
        auto constexpr const sequence
          = std::make_integer_sequence<unsigned, sizeof...(Results)>{};
        // Invoke the callback with the tuple returned
        // from the previous callback.
        applyTuple(sequence, std::move(result),
                   std::forward<decltype(next)>(next));
      };
    };
  }
};

/// Create the proxy callback that is responsible for invoking
/// the real callback and passing the next continuation into
/// the result of the following callback.
template<typename Callback, typename Next>
auto createProxyCallback(Callback&& callback,
                         Next&& next) {
  return [callback = std::forward<Callback>(callback),
          next = std::forward<Next>(next)] (auto&&... args) mutable {
    // Callbacks shall always return a continuation,
    // if not, we need to decorate it.
    using Result = decltype(callback(std::forward<decltype(args)>(args)...));
    using Decorator = CallbackResultDecorator<Result>;
    Decorator::decorate(std::move(callback))
            (std::forward<decltype(args)>(args)...)(std::move(next));
  };
}

template<typename Continuation, typename Callback>
auto appendCallback(Continuation&& continuation,
                    Callback&& callback) {
  return [continuation = std::forward<Continuation>(continuation),
          callback = std::forward<Callback>(callback)](auto&& next) mutable {
    // Invoke the next invocation handler
    std::move(continuation)(createProxyCallback(
      std::move(callback), std::forward<decltype(next)>(next)));
  };
}

template<typename Data>
void invokeContinuation(Data data) {
  // Check whether the ownership is acquired and start the continuation call
  if (data.ownership.hasOwnership()) {
    // Pass an empty callback to the continuation to invoke it
    std::move(data.continuation)(createEmptyCallback());
  }
}

/// Holds the effective data for the continuable, like the current owner status,
/// the continuation object and the dispatcher.
template<typename ContinuationType,
         typename DispatcherType>
struct ContinuableData {
  /// The plain continuation type that is stored within the data.
  /// Continuation types have a templated or a fixed signature
  /// where `operator()` expects one variable that has the following
  /// `operator()` signature: `void()` callback args
  /// For instance:
  ///   std::function<void(std::function<void(int, float)>)>
  ///        ^^^^^^^^           ^^^^^^^^
  ///      continuation         callback
  /// where int and float are the arguments the callback is invoked with.
  using Continuation = ContinuationType;
  /// TODO
  using Dispatcher = DispatcherType;

  ContinuableData(Continuation continuation_,
    Dispatcher dispatcher_) noexcept
    : continuation(std::move(continuation_)),
    dispatcher(std::move(dispatcher_)) { }

  ContinuableData(Ownership ownership_,
                  Continuation continuation_,
                  Dispatcher dispatcher_) noexcept
    : ownership(std::move(ownership_)),
      continuation(std::move(continuation_)),
      dispatcher(std::move(dispatcher_))  { }

  template<typename NewType>
  using ChangeContinuationTo = ContinuableData<
    NewType, Dispatcher
  >;

  template<typename NewType>
  using ChangeDispatcherTo = ContinuableData<
    Continuation, NewType
  >;

  Ownership ownership;
  Continuation continuation;
  Dispatcher dispatcher;
};

/// An undecorator that undecorates nothing
struct UndecorateNone {
  template<typename /*Target*/, typename T>
  static auto undecorate(T&& data) -> std::decay_t<T> {
    return std::forward<T>(data);
  }
};

/// The Undecorateable is a container for unmaterialized
/// ContinuableData which can be accessed on demand.
template<typename Data,
         typename Undecorator = UndecorateNone>
class Undecorateable {
public:
  explicit Undecorateable(Data data_)
    : data(std::move(data_)) { }

  /// Return a r-value reference to the data
  template<typename Target>
  auto undecorate()&& {
    return Undecorator::template undecorate<Target>(std::move(data));
  }

  /// Return a copy of the data
  template<typename Target>
  auto undecorate() const& {
    return Undecorator::template undecorate<Target>(data);
  }

private:
  Data data;
};


template<typename... TargetArgs, typename... CombinedData>
auto undecorateCombined(Identity<TargetArgs...>,
                        std::tuple<CombinedData...> combined) {

}

template<typename Callback, typename... CombinedData>
auto undecorateCombined(std::tuple<CombinedData...> combined) {
  // using TargetArgs = typename do_undecorate<Callback>::argument_type;
  // return undecorateCombined(TargetArgs{}, std::move(combined));
}

// FIXME
template<typename Combined>
class LazyCombineDecoration {
public:
  // TODO
  explicit LazyCombineDecoration(Combined combined_)
    : combined(std::move(combined_)) { }

  // using Config = typename Data::Config;

  template<typename Callback>
  static void requiresUndecorateable() {
    static_assert(is_undecorateable<Callback>::value,
                  "Can't retrieve the signature of the given callback. "
                  "Consider to pass an untemplated function or functor "
                  "to the `then` method invocation to fix this.");
  }

  /// Return a r-value reference to the data
  template<typename Callback>
  void undecorate()&& {
    requiresUndecorateable<Callback>();
    return undecorateCombined<Callback>(std::move(combined));
  }

  template<typename Callback>
  /// Return a copy of the data
  void undecorate() const& {
    requiresUndecorateable<Callback>();
    return undecorateCombined<Callback>(combined);
  }

  template<typename RightLazyCombine>
  auto merge(RightLazyCombine right)&& {
    auto merged = tupleMerge(std::move(combined), std::move(right.combined));
    return ContinuableBase<LazyCombineDecoration<decltype(merged)>> {
      LazyCombineDecoration<decltype(merged)>{std::move(merged)}
    };
  }

private:
  Combined combined;
};

template<typename Continuation, typename Dispatcher = SelfDispatcher>
auto make_continuable(Continuation&& continuation,
                      Dispatcher&& dispatcher = SelfDispatcher{}) noexcept {
  using Decoration = Undecorateable<ContinuableData<
    std::decay_t<Continuation>,
    std::decay_t<Dispatcher>
  >>;
  return ContinuableBase<Decoration>(Decoration({
    std::forward<Continuation>(continuation),
    std::forward<Dispatcher>(dispatcher)
  }));
}

template<typename Undecorated, typename Callback>
auto thenImpl(Undecorated undecorated, Callback&& callback) {
  auto next = appendCallback(std::move(undecorated.continuation),
                             std::forward<Callback>(callback));
  using Decoration = Undecorateable<
    typename Undecorated::template ChangeContinuationTo<decltype(next)>
  >;
  return ContinuableBase<Decoration>(Decoration({
    std::move(undecorated.ownership),
    std::move(next),
    std::move(undecorated.dispatcher)
  }));
}

template<typename Undecorated, typename NewDispatcher>
auto postImpl(Undecorated undecorated, NewDispatcher&& newDispatcher) {
  using Decoration = Undecorateable<
    typename Undecorated::template
      ChangeDispatcherTo<std::decay_t<NewDispatcher>>
  >;
  return ContinuableBase<Decoration>(Decoration({
    std::move(undecorated.ownership),
    std::move(undecorated.continuation),
    std::forward<NewDispatcher>(newDispatcher)
  }));
}

template<typename Combined>
auto toLazyCombined(LazyCombineDecoration<Combined> decoration) {
  return std::move(decoration);
}

template<typename Decoration>
auto toLazyCombined(Decoration&& decoration) {
  auto data = std::forward<Decoration>(decoration).template undecorate<void>();
  return LazyCombineDecoration<std::tuple<decltype(data)>>(std::move(data));
}

template<typename LeftDecoration, typename RightDecoration>
auto combineImpl(LeftDecoration&& leftDecoration,
                 RightDecoration&& rightDecoration) {
  return toLazyCombined(std::forward<LeftDecoration>(leftDecoration))
    .merge(toLazyCombined(std::forward<RightDecoration>(rightDecoration)));
 }

template<typename Decoration>
class ContinuableBase {
  template<typename>
  friend class ContinuableBase;

public:
  explicit ContinuableBase(Decoration decoration_)
    : decoration(std::move(decoration_)) { }

  ~ContinuableBase() {
    // Undecorate/materialize the decoration
    // invokeContinuation(std::move(decoration).template undecorate<void>());
  }
  ContinuableBase(ContinuableBase&&) = default;
  ContinuableBase(ContinuableBase const&) = default;

  template<typename Callback>
  auto then(Callback&& callback)&& {
    return thenImpl(std::move(decoration).template undecorate<Callback>(),
                    std::forward<Callback>(callback));
  }

  template<typename Callback>
  auto then(Callback&& callback) const& {
    return thenImpl(decoration.template undecorate<Callback>(),
                    std::forward<Callback>(callback));
  }

  /*template<typename NewDispatcher>
  auto post(NewDispatcher&& newDispatcher)&& {
    return postImpl(std::move(decoration).template undecorate<void>(),
                    std::forward<NewDispatcher>(newDispatcher));
  }

  template<typename NewDispatcher>
  auto post(NewDispatcher&& newDispatcher) const& {
    return postImpl(decoration.template undecorate<void>(),
                    std::forward<NewDispatcher>(newDispatcher));
  }*/

  template<typename RightDecoration>
  auto operator&& (ContinuableBase<RightDecoration> right)&& {
    return combineImpl(std::move(decoration), std::move(right.decoration));
  }

  template<typename RightDecoration>
  auto operator&& (ContinuableBase<RightDecoration> right) const& {
    return combineImpl(decoration, std::move(right.decoration));
  }

  template<typename Callback>
  auto undecorateFor(Callback&&) {
    return decoration.template undecorate<Callback>();
  }

private:
  /// The Decoration represents the possible lazy materialized
  /// data of the continuable.
  /// The decoration pattern is used to make it possible to allow lazy chaining
  /// of operators on Continuables like the and expression `&&`,
  /// that requires lazy evaluation and the signature of the callback chained
  /// with ContinuableBase::then.
  Decoration decoration;
};

static auto makeTestContinuation() {
  return make_continuable([i = std::make_unique<int>(0)](auto&& callback) {
    callback("47");
  });
}

struct Inspector {
  template<typename... Args>
  auto operator() (Args...) {
    return std::common_type<std::tuple<Args...>>{};
  }
};

template<unsigned N>
struct FailIfWrongArgs {
  template<typename... Args>
  auto operator() (Args...)
    -> std::enable_if_t<N == sizeof...(Args)> { }
};

#endif // CONTINUABLE_HPP_INCLUDED__
