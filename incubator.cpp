
#include <functional>

#include "continuable/continuable.hpp"


template<typename... Args>
using continuable = decltype(make_continuable(std::declval<std::function<void(Args...)>>));

static auto makeTestContinuation() {
  return make_continuable([i = std::make_unique<int>(0)](auto&& callback) {
    callback("47");
  });
}

static auto dnsResolve(std::string) {
  return make_continuable([](auto&& callback) {
    callback("");
  });
}

static auto httpRequestToIp(std::string) {
  return make_continuable([](auto&& callback) {
    callback("");
  });
}

template<typename T>
static auto flatten(T&& factory) {
  return [factory = std::forward<T>(factory)](auto&&... args) {
    return factory(std::forward<decltype(args)>(args)...);
  };
}

/*static auto httpRequest(std::string url) {
  return dnsResolve(url)
    .then(flatten(httpRequestToIp))
    .then([](std::string result) {
      
    });
}*/

struct Debugable {
  int i = 5;

  template<typename C>
  void operator() (C&& c) {
	// fail<C> cc;
    std::forward<C>(c)(true);
  }
};

static auto moveTo() {
  return make_continuable([](auto&& callback) {
    callback(true);
  });

  // return make_continuable(Debugable{});
}

int main(int, char**) {

  Debugable deb;

  auto empty = [](auto&&...) {};

  deb(empty);

  moveTo()
    .then([](bool) {
	    return make_continuable(Debugable{});
    })
    .then([](bool) {

    });

  // continuable<int, int> c;

  // auto dispatcher = SelfDispatcher{};

  /*(makeTestContinuation() && makeTestContinuation())
    .undecorateFor([]()
    {

    });*/

  /*auto unwrapper = [](auto&&... args) {
    return std::common_type<std::tuple<decltype(args)...>>{};
  };*/

  // using T = decltype(unwrap(FailIfWrongArgs<0>{}));

  // using T = decltype(unwrap(std::declval<Inspector>()));
  // T t{};

  // auto combined = makeTestContinuation() && makeTestContinuation();

  int res = 0;
  /*makeTestContinuation()
    .then([](std::string) {
      return std::make_tuple(47, 46, 45);
    })
    // .post(dispatcher)
    .then([](int val1, int val2, int val3) {
      return val1 + val2 + val3;
    })
    .then([&](int val) {
      res += val;
    })
    .then([] {
      return makeTestContinuation();
    })
    .then(makeTestContinuation())
    .then([] (std::string arg) {
      arg.clear();
    });*/

  return res;
}
