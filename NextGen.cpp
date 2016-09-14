
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

#include <tuple>
#include <type_traits>
#include <string>
#include <memory>

struct SelfDispatcher {
  template<typename T>
  void operator() (T&& callable) const {
    std::forward<T>(callable)();
  }
};

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

class Ownership {
public:
  Ownership() { }
  Ownership(Ownership const&) = default;
  explicit Ownership(Ownership&& right) noexcept
    : isOwningThis(takeOverFrom(std::move(right))) { };
  Ownership& operator = (Ownership const&) = default;
  Ownership& operator = (Ownership&& right) noexcept {
    isOwningThis = takeOverFrom(std::move(right));
    return *this;
  }

  bool IsOwning() const noexcept {
    return isOwningThis;
  }

private:
  bool static takeOverFrom(Ownership&& right) {
    bool value = right.isOwningThis;
    right.isOwningThis = false;
    return value;
  }

  bool isOwningThis{ true };
};

/// Decorates none
template<typename Result>
struct CallbackResultDecorator {
  template<typename Callback>
  static auto decorate(Callback&& callback) {
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
/// to the result of the callback.
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
auto appendHandlerToContinuation(Continuation&& continuation,
                                 Callback&& callback) {

  return [continuation = std::forward<Continuation>(continuation),
          callback = std::forward<Callback>(callback)](auto&& next) mutable {
    // Invoke the next invocation handler
    std::move(continuation)(createProxyCallback(
      std::move(callback), std::forward<decltype(next)>(next)));
  };
}

template<typename Continuation>
void invokeContinuation(Continuation&& continuation) {
  // Pass an empty callback to the continuation to invoke it
  std::forward<Continuation>(continuation)(createEmptyCallback());
}

template<typename ContinuationType, typename DispatcherType>
struct ContinuableConfig {
  using Continuation = ContinuationType;
  using Dispatcher = DispatcherType;

  template<typename NewType>
  using ChangeContinuationTo = ContinuableConfig<
    NewType, Dispatcher
  >;

  template<typename NewType>
  using ChangeDispatcherTo = ContinuableConfig<
    Continuation, NewType
  >;
};

template<typename Continuation, typename Dispatcher = SelfDispatcher>
auto make_continuable(Continuation&& continuation,
                      Dispatcher&& dispatcher = SelfDispatcher{}) noexcept {
  using Config = ContinuableConfig<
    std::decay_t<Continuation>,
    std::decay_t<Dispatcher>
  >;
  return ContinuableBase<Config> {
    std::forward<Continuation>(continuation),
    std::forward<Dispatcher>(dispatcher)
  };
}

template<typename Config>
class ContinuableBase {
  template<typename>
  friend class ContinuableBase;

  ContinuableBase(typename Config::Continuation continuation_,
                  Ownership ownership_,
                  typename Config::Dispatcher dispatcher_) noexcept
    : continuation(std::move(continuation_)),
      dispatcher(std::move(dispatcher_)), ownership(std::move(ownership_)) { }

public:
  ContinuableBase(typename Config::Continuation continuation_,
                  typename Config::Dispatcher dispatcher_) noexcept
    : continuation(std::move(continuation_)),
      dispatcher(std::move(dispatcher_)) { }
  ~ContinuableBase() {
    if (ownership.IsOwning()) {
      invokeContinuation(std::move(continuation));
    }
  }
  ContinuableBase(ContinuableBase&&) = default;
  ContinuableBase(ContinuableBase const&) = default;

  template<typename Callback>
  auto then(Callback&& callback)&& {
    auto next = appendHandlerToContinuation(std::move(continuation),
                                            std::forward<Callback>(callback));
    using Transformed = ContinuableBase<
      typename Config::template ChangeContinuationTo<decltype(next)>
    >;
    return Transformed {
      std::move(next), std::move(ownership), std::move(dispatcher)
    };
  }

  template<typename Callback>
  auto then(Callback&& callback) const& {
    auto next = appendHandlerToContinuation(continuation,
                                            std::forward<Callback>(callback));
    using Transformed = ContinuableBase<
      typename Config::template ChangeContinuationTo<decltype(next)>
    >;
    return Transformed {
      std::move(next), ownership, dispatcher
    };
  }

  template<typename NewDispatcher>
  auto post(NewDispatcher&& newDispatcher)&& {
    using Transformed = ContinuableBase<
      typename Config::template ChangeDispatcherTo<std::decay_t<NewDispatcher>>
    >;
    return Transformed {
      std::move(continuation), std::move(ownership),
      std::forward<NewDispatcher>(newDispatcher)
    };
  }

  template<typename NewDispatcher>
  auto post(NewDispatcher&& newDispatcher) const& {
    using Transformed = ContinuableBase<
      typename Config::template ChangeDispatcherTo<std::decay_t<NewDispatcher>>
    >;
    return Transformed {
      continuation, ownership,
      std::forward<NewDispatcher>(newDispatcher)
    };
  }

private:
  typename Config::Continuation continuation;
  typename Config::Dispatcher dispatcher;
  Ownership ownership;
};

static auto makeTestContinuation() {
  return make_continuable([](auto&& callback) {
    callback("<br>hi<br>");
  });
}

int main(int, char**) {
  auto dispatcher = SelfDispatcher{};

  int res = 0;
  makeTestContinuation()
    .post(dispatcher)
    .then([](std::string) {
      

      return std::make_tuple(47, 46, 45);
    })
    .then([&](int val1, int val2, int val3) {


      res += val1 + val2 + val3;
      int i = 0;
    });

  return res;
}
