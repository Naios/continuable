
<p align="center">
  <a href="https://naios.github.io/continuable/">
    <img alt="Continuable" src="https://raw.githubusercontent.com/Naios/continuable/master/doc/slideshow.gif">
  </a>
</p>

<p align="center">
  <a href="https://naios.github.io/continuable/changelog.html#changelog-versions-4-0-0"><img alt="Current version" src="https://img.shields.io/badge/Version-4.0.0-0091EA.svg"></a>
  <a href="https://travis-ci.org/Naios/continuable"><img alt="Travic-CI build status" src="https://travis-ci.org/Naios/continuable.svg?branch=master"></a>
  <a href="https://ci.appveyor.com/project/Naios/continuable/branch/master"><img alt="AppVeyor CI status" src="https://ci.appveyor.com/api/projects/status/328ta3r5x92f3byv/branch/master?svg=true"></a>
  <img alt="MIT Licensed" src="https://img.shields.io/badge/License-MIT-00838F.svg">
  <a href="https://naios.github.io/continuable/"><img alt="Documentation" src="https://img.shields.io/badge/Documentation-Doxygen-26A69A.svg"></a>
  <a href="https://wandbox.org/permlink/EDr7u2P5HXs2W6p1"><img alt="Try continuable online" src="https://img.shields.io/badge/Run-online-4DB6AC.svg"></a>
  <a href="https://godbolt.org/g/iyE4Ww"><img alt="Compiler explorer" src="https://img.shields.io/badge/Compiler-explorer-58CEC2.svg"></a>
</p>

------

#### Continuable is a C++14 library that provides full support for:

* lazy async continuation chaining based on callbacks (**then**) and expression templates, callbacks are wrapped nicely as **promises**.
* **no enforced type-erasure** which means we need **less heap allocations** than comparable libraries, strictly following the **"don't pay for what you don't use"** principle.
* support for *all*, *any* and *sequential* connections between continuables through expressive operator overloads **&&**, **||** and **>>** as well as free functions **when_all**, **when_any** and **when_seq**.
* asynchronous **error handling** through **exceptions**, **error codes** and **user defined types**.
* syntactic sugar for instance: **partial invocation**, **tuple unpacking**, `co_await` support and **executors**.
* **encapsuled from any runtime**, larger framework or executors makes it possible to use continuable even in smaller or esoteric usage scenarios.

------

#### Getting started:

The [documentation](https://naios.github.io/continuable/) offers everything you need:
* [Installation guide](https://naios.github.io/continuable/installation.html)
* [Usage tutorial](https://naios.github.io/continuable/tutorial.html)
* [Configuration explanation](https://naios.github.io/continuable/configuration.html)
* [Changelog](https://naios.github.io/continuable/changelog.html)


#### Issues and contributions

Issue reports and questions are accepted through the Github issue tracker as well as pull requests.
Every contribution is welcome! Don't hesitate to ask for help if you need any support
in improving the implementation or if you have any troubles in using the library

#### Quick Tour

- **Create a continuable through `make_continuable` which returns a promise on invocation:**
  ```cpp
  auto http_request(std::string url) {
    return cti::make_continuable<std::string>([url = std::move(url)](auto&& promise) {
      // Perform the actual request through a different library,
      // resolve the promise upon completion of the task.
      promise.set_value("<html> ... </html>");
      // or: promise.set_exception(std::make_exception_ptr(std::exception("Some error")));
      // or: promise.set_canceled();
    });
  }

  auto mysql_query(std::string query) {
    return cti::make_continuable<result_set, bool>([url = std::move(url)](auto&& promise) {
      //                         ^^^^^^^^^^^^^^ multiple result types
    });
  }

  auto do_sth() {
    return cti::make_continuable<void>([](auto&& promise) {
      //                         ^^^^ no result at all
    });
  }

  auto run_it() {
    return async([] {
      // Directly start with a handler
    });
  }

  continuable<> run_it() { // With type erasure
    return async([] {

    });
  }
  ```

- **Attach your continuations through `then`, supports multiple results and partial handlers:**
  ```cpp
  mysql_query("SELECT `id`, `name` FROM `users`")
    .then([](result_set users) {
      // Return the next continuable to process ...
      return mysql_query("SELECT `id` name FROM `sessions`");
    })
    .then([](result_set sessions) {
      // ... or pass multiple values to the next callback using tuples or pairs ...
      return std::make_tuple(std::move(sessions), true);
    })
    .then([](result_set sessions, bool is_ok) {
      // ... or pass a single value to the next callback ...
      return 10;
    })
    .then([](auto value) {
      //     ^^^^ Templated callbacks are possible too
    })
    // ... you may even pass continuables to the `then` method directly:
    .then(mysql_query("SELECT * `statistics`"))
    .then([](result_set result) {
      // ...
        return "Hi";
    })
    .then([] /*(std::string result) */ { // Handlers can accept a partial set of arguments{
      // ...
    });
  ```

- **Handle failures through `fail` or `next`:**
  ```cpp
  http_request("example.com")
    .then([] {
      throw std::exception("Some error");
    })
    .fail([] (std::exception_ptr ptr) {
      if (ptr) {
        try {
          std::rethrow_exception(ptr);
        } catch(std::exception const& e) {
          // Handle the exception or error code here
        }
      }
    });
  ```

- **Dispatch continuations through a specific executor** (possibly on a different thread or later)

  ```cpp
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

- **Connect continuables through `when_all`, `when_any` or `when_seq`:**
  ```cpp
  // `all` of connections:
  (http_request("github.com") && http_request("example.com") && http_request("wikipedia.org"))
    .then([](std::string github, std::string example, std::string wikipedia) {
      // The callback is called with the response of github,
      // example and wikipedia.
    });

  // `any` of connections:
  (http_request("github.com") || http_request("example.com") || http_request("wikipedia.org"))
    .then([](std::string github_or_example_or_wikipedia) {
      // The callback is called with the first response of either github,
      // example or wikipedia.
    });

  // `sequence` of connections:
  (http_request("github.com") >> http_request("example.com") >> http_request("wikipedia.org"))
    .then([](std::string github, std::string example, std::string wikipedia) {
      // The requests are invoked sequentially
    });

  // Mixed logical connections:
  (http_request("github.com") && (http_request("example.com") || http_request("wikipedia.org")))
    .then([](std::string github, std::string example_or_wikipedia) {
      // The callback is called with the response of github for sure
      // and the second parameter represents the response of example or wikipedia.
    });

  // There are helper functions for connecting continuables:
  auto all = cti::when_all(http_request("github.com"), http_request("example.com"));
  auto any = cti::when_any(http_request("github.com"), http_request("example.com"));
  auto seq = cti::when_seq(http_request("github.com"), http_request("example.com"));
  ```

- **Deal with multiple result variables through `result` and `recover` from failures:**
  ```cpp
  make_exceptional_continuable<void>(std::make_exception_ptr(std::exception("Some error"))
    .fail([] (std::exception_ptr ptr) {
      return recover();
    })
    .then([] () -> result<> {
      // We recovered from the failure and proceeding normally

      // Will yield a default constructed exception type to signal cancellation
      return cancel();
    });
  ```

- **`promisify` your existing code or use the (asio) completion token integration:**
  ```cpp
  // Promisification of your existing code that accepts callbacks
  auto async_resolve(std::string host, std::string service) {
    return cti::promisify<asio::ip::udp::resolver::iterator>::from(
        [&](auto&&... args) {
          resolver_.async_resolve(std::forward<decltype(args)>(args)...);
        },
        std::move(host), std::move(service));
  }

  // (boost) asio completion token integration
  asio::io_context io_context;
  asio::steady_timer steady_timer(io_context);

  steady_timer.expires_after(std::chrono::seconds(5));
  steady_timer.async_wait(cti::use_continuable)
    .then([] {
      // Is called after 5s
    });
  ```

- **C++20 Coroutine support:**

  (`co_await` and `co_return`) are supported by continuable when the underlying toolchain supports the TS. Currently this works in MSVC 2017 and Clang 5.0. You have to enable this capability through the `CTI_CONTINUABLE_WITH_AWAIT` define in CMake:

  ```cpp
  int i = co_await cti::make_continuable<int>([](auto&& promise) {
    promise.set_value(0);
  });
  ```


#### Appearances:

| [MeetingC++ 2018 Talk](https://naios.github.io/talks/2018-11-17-Meeting-C%2B%2B-Berlin/Continuable.pdf) |
| :---: |
| [<img alt="Continuable MeetingC++" width="60%" src="https://img.youtube.com/vi/l6-spMA_x6g/0.jpg">](https://www.youtube.com/watch?v=l6-spMA_x6g)] |

.
