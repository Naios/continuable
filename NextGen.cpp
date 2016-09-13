
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

auto createEmptyContinuation() {
  return [](auto&& callback) { callback(); };
}

auto createEmptyCallback() {
  return [](auto&&...) { };
}

template<typename Result>
struct CallbackResultDecorator {
  template<typename Callback>
  static auto decorate(Callback&& callback) {
    return std::forward<Callback>(callback);
  }
};

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

// Create the proxy callback that is responsible for invoking
// the real callback and passing the next continuation into
// to the result of the callback.
template<typename Callback, typename Next>
auto createProxyCallback(Callback&& callback,
                         Next&& next) {
  return [callback = std::forward<Callback>(callback),
          next = std::forward<Next>(next)] (auto&&... args) mutable {
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

auto makeTestContinuation() {
  return [](auto&& callback) {
    callback("<br>hi<br>");
  };
}

int main(int, char**) {
  auto continuation = makeTestContinuation();

  auto then1 = [](std::string) {
    int i = 0;
  };

  auto then2 = []() {

    int i = 0;
  };

  auto f1 = appendHandlerToContinuation(continuation, then1);
  auto f2 = appendHandlerToContinuation(f1, then2);

  invokeContinuation(f2);
}
