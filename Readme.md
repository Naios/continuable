# continuable->then(make_things_simple());

[![Build Status](https://travis-ci.org/Naios/continuable.svg?branch=master)](https://travis-ci.org/Naios/continuable) ![](https://img.shields.io/badge/License-MIT-blue.svg) [![](https://img.shields.io/badge/Try-online-green.svg)](http://melpon.org/wandbox/permlink/TPSde7EsCcXcC72D)

> Async C++14 platform independent continuation chainer providing light and allocation aware futures

This library provides full feature support of:

* async continuation chaining using **callbacks** (*then*).
* **no enforced type-erasure** which means we need **extremely fewer heap allocations** .
* support for **finite logical connections** between continuables through an **all or any** strategy.
* **syntactic sugar** for attaching callbacks to a continuation.




## The library design

The continuable library was designed in order to provide you as much as flexibility as possible:

- There is no enforced type erasure which means there is less memory allocation and thus the callback chains are heavily optimizable by the compiler. That's why the library is well usable in the embedded or gaming field. **Don't pay for what you don't use!**
- The library provides support for **dispatching callbacks on a given executor**, however, it doesn't provide it's own one. You probably will use your own executor like [asio](https://github.com/chriskohlhoff/asio), [libuv](https://github.com/libuv/libuv) or a corresponding [lock-free concurrentqueue](https://github.com/cameron314/concurrentqueue) anyway. In most cases, the executor will do the type erasure for you, so there is no reason to do it twice.
- The library provides as much as **syntactic sugar** as it's possible, in order to make continuation chaining expressive and simple. For instance, it allows you to logical connect continuables through the well-known operators `&&` and `||`.
- The library is header only and has **fewer dependencies**:
  - The `continuable-base.hpp` header only depends on the standard library and provides the basic continuation logic.
  - The `continuable-test.hpp` header also depends on `gtest` because it adds various test macros for asserting on the result of asynchronous continuations.
  - The `continuable.hpp`header depends on my header-only [function2](https://github.com/Naios/function2) library for providing efficient type erasure - non-copyable objects are natively supported without any workaround.


## Installation

### Inclusion

As mentioned earlier the library is header-only. There is a cmake project provided for simple setup:

```sh
# Shell:
git submodule add https://github.com/Naios/continuable.git
```

```cma
# CMake file:
add_subdirectory(continuable)
# continuable provides an interface target which makes it's
# headers available to all projects using the continuable library.
target_link_libraries(my_project continuable)
```

On POSIX you are required to link your application against a corresponding thread library, otherwise `std:: future's` won't work properly, this is done automatically by the provided cmake project.

### Building the unit-tests

In order to build the unit tests clone the repository recursively with all submodules:

```sh
# Shell:
git clone --recursive https://github.com/Naios/continuable.git
```
## Stability and version

Currently, the library is in the incubation state, it provides a stable functionality as the CI unit tests indicate.

The API isn't fixed right now and will be eventually changed into the future.

Also, the unit-test is observed with the Clang sanitizers (asan, ubsan and lsan - when compiling with Clang) or Valgrind (when compiling with GCC).

## Quick reference

### Creating Continuables 

Create a continuable from a callback taking function:

```c++
#include "continuable/continuable-base.hpp"
// ...

auto void_continuable = cti::make_continuable<void>([](auto&& callback) {
  //                                          ^^^^
  
  // Call the callback later when you have finished your work
  callback();
});

auto str_continuable = cti::make_continuable<std::string>([](auto&& callback) {
  //                                         ^^^^^^^^^^^
  callback("Hello, World!");
});
```

### Chaining Continuables

Chain continuables together in order to build up an asynchronous call hierarchy:

```c++
mysql_query("SELECT `id`, `name` FROM `users`")
  .then([](ResultSet users) {
    // Return the next continuable to process ...
    return mysql_query("SELECT `id` name FROM `sessions`");   
  })
  .then([](ResultSet sessions) {
    // ... or pass multiple values to the next callback using tuples or pairs ...
    return std::make_tuple(std::move(sessions), true);
  })
  .then([](ResultSet sessions, bool is_ok) {
    // ... or pass a single value to the next callback ...
    return 10;
  })
  .then([](auto value) {
    //     ^^^^ Templated callbacks are possible too
  })
  // ... you may even pass continuables to the `then` method directly:
  .then(mysql_query("SELECT * `statistics`"))
  .then([](ResultSet result) {
    // ...
  });
```


### Providing helper functions

Returning continuables from helper functions makes chaining simple and expressive:

```c++
#include "continuable/continuable-base.hpp"
// ...

auto mysql_query(std::string query) {
  return cti::make_continuable<ResultSet>([query = std::move(query)](auto&& callback) mutable {
    // Pass the callback to the handler which calls the callback when finished.
    // Every function accepting callbacks works with continuables.
    mysql_handle_async_query(std::move(query),
                             std::forward<decltype(callback)>(callback));
  });
}

// You may use the helper function like you would normally do,
// without using the support methods of the continuable.
mysql_query("DELETE FROM `users` WHERE `id` = 27361");

// Or using chaining to handle the result which is covered in the documentation.
mysql_query("SELECT `id`, `name` FROM users")
  .then([](ResultSet result) {
    // ...
  });
```

### Connecting Continuables {all or any}

Continuables provide the operators **&&** and **||** for logical connection:

* **&&** invokes the final callback with the compound result of all connected continuables.
* **||** invokes the final callback once with the first result available.

```C++
auto http_request(std::string url) {
  return cti::make_continuable<std::string>([](auto&& callback) {
    callback("<html>...</html>");
  });
}

// `all` of connections:
(http_request("github.com") && http_request("travis-ci.org") && http_request("atom.io"))
  .then([](std::string github, std::string travis, std::string atom) {
    // The callback is called with the response of github, travis and atom.
  });

// `any` of connections:
(http_request("github.com") || http_request("travis-ci.org") || http_request("atom.io"))
  .then([](std::string github_or_travis_or_atom) {
    // The callback is called with the first response of either github, travis or atom.
  });

// mixed logical connections:
(http_request("github.com") && (http_request("travis-ci.org") || http_request("atom.io")))
  .then([](std::string github, std::string travis_or_atom) {
    // The callback is called with the response of github for sure
    // and the second parameter represents the response of travis or atom.
  });

// There are helper functions for connecting continuables:
auto all = cti::all_of(http_request("github.com"), http_request("travis-ci.org"));
auto any = cti::any_of(http_request("github.com"), http_request("travis-ci.org"));
```

Logical connections are ensured to be **thread-safe** and **wait-free** by library design (when assuming that *std::call_once* is wait-free - which depends on the toolchain).

### Type erasure

The library was designed in order to avoid type-erasure until really needed. Thus we provide traits to create an alias to a continuable using the **type-erasure backend of your choice**. All templated functors providing a call operator may be used as a backend (*std::function* for instance).

The library provides aliases for using my [function2 library](https://github.com/Naios/function2) as backend which provides efficient and qualifier correct function wrappers for copyable and non-copyable objects.

```c++
#include "continuable/continuable.hpp"
// ...

cti::unique_continuable<int, std::string> unique =
      cti::make_continuable([value = std::make_unique<int>(0)](auto&& callback) {

  // The use of non copyable objects is possible by design if
  // the type erasure backend supports it.
  callback(*value, "Hello, World!");
});

std::move(unique).then([](int i) {
  // ...
});
```

However you may still define your own continuation wrapper with the backend of your choice, but keep in mind that the capabilities of your wrapper determine the possible objects, the continuation is capable of carrying. This limits continuations using *std::function* as a backend to copyable types:

```c++
template <typename... Args>
using mycontinuation = cti::continuable_of_t<
  cti::continuable_erasure_of_t<std::function, std::function, Args...>,
  Args...>;

// ...

mycontinuation<int> myc = cti::make_continuable([](auto&& callback) {
  //                                         ^^^^^
  // Signatures may be omitted for continuables which are type erased
  callback(0);
});
```

We could also think about using  `std::future` as backend but this is even worse then using `std::function` because usually there is, even more, type erasure and allocations involved.

### Future conversion

The library is capable of converting (*futurizing*) every continuable into a fitting **std::future** through the `continuable<...>::futurize()` method.

```c++
std::future<std::string> future = http_request("github.com")
  .then([](std::string response) {
    // Do sth...
    return http_request("travis-ci.org") || http_request("atom.io");
  })
  .futurize();
// ^^^^^^^^

std::future<std::tuple<std::string, std::string>> future =
  (http_request("travis-ci.org") && http_request("atom.io")).futurize();
```

Continuables returning nothing will evaluate to: `std::future<void>`.

Continuables returning only one value will evaluate the corresponding future: `std::future<type>`.

Continuables returning more then one value will evaluate to a future providing a tuple carrying the values : `std::future<std::tuple<...>>`.

### In Progress (ToDo-List)

Although the library has progressed very far there are still some candies missing:

- [ ] **Partial application**:

      We could allow callbacks to be invoked with fewer arguments than expected:

      ```C++
      http_request("github.com")
        .then([]() { // ERROR: Because we expect an object accepting a std::string
          // ...
        });
      ```


- [ ] The **sequential/pipe operator** which invokes continuables sequentially and calls the callback with all results:

      ```c++
      (http_request("github.com") | http_request("travis-ci.org") | http_request("atom.io"))
        .then([](std::string github, std::string travis, std::string atom) {
          // The requests are done sequentially and the callback is called
          // with the response of github, travis and atom as soon as atom has responded.
          // The responses of github and travis are stored meanwhile.
        });

      auto seq = cti::seq_of(http_request("github.com"), http_request("travis-ci.org"));
      ```

      This differs from the `all` connection in the way that the continuables are invoked sequentially instead of parallel.


- [ ] **Inplace resolution** (makes it possible to keep the nesting level flat):

      ```c++
      http_request("github.com")
        .then([](std::string response) {
          // Do something with the response
          int response_code = get_code(response);

          return std::make_tuple(http_request("atom.io"), response_code);
          //                     ^^^^^^^^^^^^^^^^^^^^^^^
        })
        .then([](std::string atom, int response_code) {
          // - `std::string atom` was resolved from `http_request("atom.io")`
          // - `response_code` was passed through the tuple directly
        });
      ```

- [ ] Library support of **infinite logical connections**:

      ```c++
      std::vector<cti::continuable<int, int>> some;

      cti::all(std::move(some))
        .then([](std::vector<int, int> result) {
          // ...
        });
      ```

      This library mainly aims to support un-erased continuations, however, sometimes it's required to work with a compile-time unknown amount of continuables.

- [ ] Maybe **Un-erasured fail/rejection handlers** and (possible exception support):

      ```c++
      http_request("github.com")
        .rejected([](std::error_code) {
          // Is called when the request fails

          // Potential difficult to implement with less type erasure
        });
      ```




## Compatibility

Tested & compatible with:

- Visual Studio 2015+ Update 3
- Clang 3.6+
- GCC 5.0+

Every compiler with modern C++14 support should work.

The library only depends on the standard library when using the `continuable/continuable-base.hpp` header only which provides the full un-erasured support.



On Posix: don't  forget to **link a corresponding thread library** into your application otherwise `std::future's` won't work `(-pthread)`.

## Similar implementations and alternatives

You already thought it, the idea isn't new and thus it is well known in the JavaScript world already.

There are some existing solutions with similar design thoughts already, which I don't want to hold back from you - you should choose the library fitting your needs best:

#### **[q (C++)](https://github.com/grantila/q)**

Is designed in a similar way, however, it orientates itself more on the corresponding JavaScript libraries which leaves some benefits behind we could reach with modern C++ meta-programming. Like previous approaches, the library uses type erasure excessively (and thus isn't memory allocation aware). What differentiates **q** from the continuable library is that it supports infinite logical connections and ships with built-in exception support as well as it's own executors (thread pools) - thus the library isn't header-only anymore (but the library is still proclaimed to work with other executors). My personal opinion is that a continuation library shouldn't include any other stuff then the continuation logic itself.

### [cpprestsdk](https://github.com/Microsoft/cpprestsdk)

Basically, the same arguments as explained in the q section apply to the cpprestsdk as well, it's major drawbacks is the overwhelming use of type-erasure. Probably you will benefit a lot from the library if you intend to use it's provided asynchronously *http*, *websocket* and *filesystem* functionalities. The *continuable* library was designed with different thoughts in mind - it basically provides the continuation logic without any support methods so you can embed it into your application without depending on a heavy framework. This makes it possible to apply continuation chaning to esoteric domains such as C++ AI scripts with fast or immediately response times. Who knows - maybe someone will provide *continuable* wrappers for open-source libraries like *asio*, libuv or *uWebSockets* in the future too.

### Others

If I forget to mention a library here let me know, so we can add the alternatives.



## License

The continuable library is licensed under the MIT License