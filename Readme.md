![](https://raw.githubusercontent.com/Naios/continuable/master/doc/slideshow.gif)

[![](https://img.shields.io/badge/Beta-v2.0.0-0091EA.svg)](https://github.com/Naios/continuable/releases/tag/2.0.0) [![Build Status](https://travis-ci.org/Naios/continuable.svg?branch=master)](https://travis-ci.org/Naios/continuable) [![Build status](https://ci.appveyor.com/api/projects/status/328ta3r5x92f3byv/branch/master?svg=true)](https://ci.appveyor.com/project/Naios/continuable/branch/master) ![](https://img.shields.io/badge/License-MIT-00838F.svg) [![](https://img.shields.io/badge/Documentation-Doxygen-26A69A.svg)](https://naios.github.io/continuable/) [![](https://img.shields.io/badge/Try-online-4DB6AC.svg)](http://melpon.org/wandbox/permlink/xVM2szjDLEge3YLV)

------

**Note: This readme refers to the in-progress version 2.0.0**



This library provides full feature support of:

* lazy async continuation chaining based on **callbacks** (*then*) and expression templates.
* **no enforced type-erasure** which means we need **less heap allocations**, strictly following the **"don't pay for what you don't use" ** principle. 
* support for **connections** between continuables through an **all, any or sequential** strategy through expressive operator overloads **&&**, **||** and **>>**.
* **error handling** through exceptions or custom types.
* **syntactic sugar** for instance: **partial invocation**, **tuple unpacking** and **executors**.



## The basics


* Supply a continuable which is invoked lazily upon request:
  ```cpp
  auto http_request(std::string url) {
    return cti::make_continuable<std::string>([url = std::move(url)](auto&& promise) {
      // Perform the actual request through a different library,
      // resolve the promise upon completion of the task.
      promise.set_value("<html> ... </html>");

      // Or promise.set_exception(...);
    });
  }
  ```
* Continue the continuation using `.then(...)` and `.fail(...)`, exceptions are passed to the first available handler:

  ```cpp
  http_request("github.com")
    .then([] (std::string result) {
      // Do something...
      return http_request("travis-ci.org")
    })
    .then([] (std::string result) {
      // Handle the response from 'travis-ci.org'
    })
    .fail([] (std::exception_ptr ptr) {
      try {
        std::rethrow_exception(ptr);
      } catch(std::exception const& e) {
        // Handle the exception or error code here
      }
    });
  ```
* Create connections between the continuables and use its compound result:
  ```cpp
  (http_request("github.com") && (http_request("travis-ci.org") || http_request("atom.io")))
    .then([](std::string github, std::string travis_or_atom) {
      // The callback is called with the response of github and travis or atom.
    });
  ```



## Table of contents

- [Installation](#installation)
    - [How-to use](#how-to-use)
    - [Building the unit-tests](#building-the-unit-tests)
- [Stability and version](#stability-and-version)
- [Quick reference](#quick-reference)
    - [Creating Continuables](#creating-continuables)
    - [Chaining Continuables](#chaining-continuables)
    - [Providing helper functions](#providing-helper-functions)
    - [Error handling](#error-handling)
    - [Connecting Continuables {all, any or sequential}](#connecting-continuables-all-any-or-sequential)
    - [Partial argument application](#partial-argument-application)
    - [Dispatching callbacks through a specific executor](#dispatching-callbacks-through-a-specific-executor)
    - [Type erasure](#type-erasure)
    - [Future conversion](#future-conversion)
- [Compatibility](#compatibility)
- [Similar implementations and alternatives](#similar-implementations-and-alternatives)
- [License](#license)


## Installation

### How-to use

As mentioned earlier the library is header-only. There is a cmake project provided for simple setup:

#### As git submodule

```sh
# Shell:
git submodule add https://github.com/Naios/continuable.git
```

```cmake
# CMake file:
add_subdirectory(continuable)
# continuable provides an interface target which makes it's
# headers available to all projects using the continuable library.
target_link_libraries(my_project continuable)
```

On POSIX platforms you are required to link your application against a corresponding thread library, otherwise `std::future's` won't work properly, this is done automatically by the provided CMake project.

#### As CMake library

Additionally the project exports a `continuable` target which is importable through CMake when installed:

```sh
mkdir build
cd build
cmake ..
cmake --build . --target INSTALL --config Release
```

`CMakeLists.txt`:

```cmake
find_package(continuable REQUIRED)
```

### Building the unit-tests

In order to build the unit tests clone the repository recursively with all submodules:

```sh
# Shell:
git clone --recursive https://github.com/Naios/continuable.git
```
## Stability and version

The library follows the rules of [semantic versioning](http://semver.org/), the API is kept stable across minor versions.

The CI driven unit-tests are observed through the Clang sanitizers (asan, ubsan and lsan - when compiling with Clang) or Valgrind (when compiling with GCC).

## Quick reference

This chapter only overflies the functionality of the continuable library, the full documentation is located at https://naios.github.io/continuable/.

### Creating Continuables 

Create a continuable from a promise taking function:

```c++
#include <continuable/continuable-base.hpp>
// ...

auto void_continuable = cti::make_continuable<void>([](auto&& promise) {
  //                                          ^^^^
  
  // Resolve the promise later when you have finished your work
  promise.set_value();
});

auto str_continuable = cti::make_continuable<std::string>([](auto&& promise) {
  //                                         ^^^^^^^^^^^
  promise.set_value("Hello, World!");
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

> **Note:** The continuation chain is invoked when the object is destructed or the `done()` method is called.

### Providing helper functions

Returning continuables from helper functions makes chaining simple and expressive:

```c++
#include <continuable/continuable-base.hpp>
// ...

auto mysql_query(std::string query) {
  return cti::make_continuable<ResultSet>([query = std::move(query)](auto&& promise) mutable {
    // Pass the callback to the handler which calls the callback when finished.
    // Every function accepting callbacks works with continuables.
    mysql_handle_async_query(std::move(query),
                             std::forward<decltype(promise)>(promise));
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

### Error handling

Continuables support asynchronous error handling through exceptions or custom error types. 

The error type will be **`std::exception_ptr`** except if one of the following definition is defined:
- **`CONTINUABLE_WITH_CUSTOM_ERROR_TYPE`**:  Define this to use a user defined error type.
- **`CONTINUABLE_WITH_NO_EXCEPTIONS`**: Define this to use **`std::error_condition`** as error type and to disable exception support. When exceptions are disabled this definition is set automatically.

Resolving a promise through an error will skip all following result handlers attached through **`then`**:

```cpp
auto get_bad_continuable(std::exception const& e) {
  return cti::make_continuable<void>([=] (auto&& promise) {
    try {
     throw e;
    } catch(...) {
      promise.set_exception(std::current_exception());
    }
  });
}
```

You may handle the exception as following:

```cpp
get_bad_continuable()
  .then([] {
    // ... never invoked
  })
  .then([] {
    // ... never invoked as well
  })
  .fail([] (std::exception_ptr e) {
    try {
      std::rethrow_exception(e);
    } catch(std::exception const& e) {
      // Handle the exception here
    }
  });
```

### Connecting Continuables {all, any or sequential}

Continuables provide the operators **&&** and **||** for logical connection:

* **&&** invokes the final callback with the compound result of all connected continuables, the continuables were invoked in parallel.
* **||** invokes the final callback once with the first result which becomes available.
* **>\>** invokes the final callback with the compound result of all connected continuables but the continuations were invokes sequentially.

```C++
auto http_request(std::string url) {
  return cti::make_continuable<std::string>([](auto&& promise) {
    promise.set_value("<html>...</html>");
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

// `sequence` of connections:
(http_request("github.com") >> http_request("travis-ci.org") >> http_request("atom.io"))
  .then([](std::string github, std::string travis, std::string atom) {
    // The requests are invoked sequentially
  });

// mixed logical connections:
(http_request("github.com") && (http_request("travis-ci.org") || http_request("atom.io")))
  .then([](std::string github, std::string travis_or_atom) {
    // The callback is called with the response of github for sure
    // and the second parameter represents the response of travis or atom.
  });

// There are helper functions for connecting continuables:
auto all = cti::when_all(http_request("github.com"), http_request("travis-ci.org"));
auto any = cti::when_any(http_request("github.com"), http_request("travis-ci.org"));
auto seq = cti::when_seq(http_request("github.com"), http_request("travis-ci.org"));
```

> **Note:** Logical connections are ensured to be **thread-safe** and **wait-free** by library design (when assuming that *std::call_once* is wait-free - which depends on the toolchain).

### Partial argument application

The callback is called only with the arguments it's accepting:

```c++
(http_request("github.com") && read_file("entries.csv"))
  .then([] {
    // ^^^^^^ The original signature was <std::string, Buffer>,
    // however, the callback is only invoked with the amount of
    // arguments it's accepting.
  });
```

### Dispatching callbacks through a specific executor

Dispatching a callback through a specific executor is supported through through the second argument of `then()`:

```c++
auto executor = [](auto&& work) {
  // Dispatch the work here, store it for later invocation or move it to another thread.
  std::forward<decltype(work)>(work)();
};

read_file("entries.csv")
  .then([](Buffer buffer) {
    // ...
  }, executor);
//   ^^^^^^^^
```

### Type erasure

The library was designed in order to avoid type-erasure until really needed. Thus we provide traits to create an alias to a continuable using the **type-erasure backend of your choice**. All templated functors providing a call operator may be used as a backend (*std::function* for instance).

The library provides aliases for using my [function2 library](https://github.com/Naios/function2) as backend which provides efficient and qualifier correct function wrappers for copyable and non-copyable objects.

```c++
#include <continuable/continuable.hpp>

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
using my_continuation = typename cti::continuable_trait<
  std::function, std::function, Args...
>::continuable;

// ...

my_continuation<int> myc = cti::make_continuable([](auto&& callback) {
  //                                         ^^^^^
  // Signatures may be omitted for continuables which are type erased
  callback(0);
});
```

We could also think about using  `std::future` as backend but this is even worse then using `std::function` because usually there is, even more, type erasure and allocations involved.

### Future conversion

The library is capable of converting (*futurizing*) every continuable into a fitting **std::future** through the `continuable<...>::futurize()` method.:

```c++
std::future<std::string> future = http_request("github.com")
  .then([](std::string response) {
    // Do sth...
    return http_request("travis-ci.org") || http_request("atom.io");
  })
  .apply(cti::transform::futurize());
// ^^^^^^^^

std::future<std::tuple<std::string, std::string>> future =
  (http_request("travis-ci.org") && http_request("atom.io")).futurize();
```

> **Note:** See the [doxygen documentation](https://naios.github.io/continuable/) for detailed information about the return type of `futurize()`.

## Compatibility

Tested & compatible with:

- Visual Studio 2017+ Update 2
- Clang 5.0+
- GCC 6.0+

Although the build is observed with the latest toolchains earlier ones might work.

The library only depends on the standard library when using the `continuable/continuable-base.hpp` header, which provides the basic continuation logic.

> **Note:** On Posix: don't  forget to **link a corresponding thread library** into your application otherwise `std::future's` won't work `(-pthread)`  when using future based transforms.

## Similar implementations and alternatives

You already thought it, the idea isn't new and thus it is well known in the JavaScript world already.

There are some existing solutions with similar design thoughts already, which I don't want to hold back from you - you should choose the library fitting your needs best:

#### **[q (C++)](https://github.com/grantila/q)**

Is designed in a similar way, however, it orientates itself more on the corresponding JavaScript libraries which leaves some benefits behind we could reach with modern C++ meta-programming. Like previous approaches, the library uses type erasure excessively (and thus isn't memory allocation aware). What differentiates **q** from the continuable library is that it supports infinite logical connections and ships with built-in exception support as well as it's own executors (thread pools) - thus the library isn't header-only anymore (but the library is still proclaimed to work with other executors). My personal opinion is that a continuation library shouldn't include any other stuff then the continuation logic itself.

#### [cpprestsdk](https://github.com/Microsoft/cpprestsdk)

Basically, the same arguments as explained in the q section apply to the cpprestsdk as well, it's major drawbacks is the overwhelming use of type-erasure. Probably you will benefit a lot from the library if you intend to use it's provided asynchronously *http*, *websocket* and *filesystem* functionalities. The *continuable* library was designed with different thoughts in mind - it basically provides the continuation logic without any support methods so you can embed it into your application without depending on a heavy framework. This makes it possible to apply continuation chaning to esoteric domains such as C++ AI scripts with fast or immediately response times. Who knows - maybe someone will provide *continuable* wrappers for open-source libraries like *asio*, libuv or *uWebSockets* in the future too.


> **Note:** If I forget to mention a library here let me know, so we can add the alternatives.


## License

The continuable library is licensed under the MIT License:

```c++
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
```

