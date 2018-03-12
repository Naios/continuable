
<p align="center">
  <a href="https://naios.github.io/continuable/">
    <img alt="Continuable" src="https://raw.githubusercontent.com/Naios/continuable/master/doc/slideshow.gif">
  </a>
</p>

<p align="center">
  <a href="https://naios.github.io/continuable/changelog.html#changelog-versions-3-0-0"><img alt="Current version" src="https://img.shields.io/badge/Version-3.0.0-0091EA.svg"></a>
  <a href="https://travis-ci.org/Naios/continuable"><img alt="Travic-CI build status" src="https://travis-ci.org/Naios/continuable.svg?branch=master"></a>
  <a href="https://ci.appveyor.com/project/Naios/continuable/branch/master"><img alt="AppVeyor CI status" src="https://ci.appveyor.com/api/projects/status/328ta3r5x92f3byv/branch/master?svg=true"></a>
  <img alt="MIT Licensed" src="https://img.shields.io/badge/License-MIT-00838F.svg">
  <a href="https://naios.github.io/continuable/"><img alt="Documentation" src="https://img.shields.io/badge/Documentation-Doxygen-26A69A.svg"></a>
  <a href="http://melpon.org/wandbox/permlink/xVM2szjDLEge3YLV"><img alt="Try continuable online" src="https://img.shields.io/badge/Try-online-4DB6AC.svg"></a>
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

Issue reports are accepted through the Github issue tracker as well as Pull requests.
Every contribution is welcome! Don't hesitate to ask for help if you need any support
in improving the implementation or if you have any troubles in using the library.
