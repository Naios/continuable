
#include <functional>

#include "continuable/continuable.hpp"

template<typename... Args>
using continuable = decltype(make_continuable(std::declval<std::function<void(Args...)>>));

static auto makeTestContinuation() {
  return make_continuable([i = std::make_unique<int>(0)](auto&& callback) {
    callback("47");
  });
}

int main(int, char**) {
  // continuable<int, int> c;

  auto dispatcher = SelfDispatcher{};

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
  makeTestContinuation()
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

    });

  return res;
}
