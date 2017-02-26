# Documentation of continuable

This documentation covers the continuable library in detail

## Content

- Class cti::continuable_base  - main class for continuation chaining
  - \link cti::continuable_base::then then\endlink - adds a callback or  cti::continuable_base to the invocation chain.
  - \link cti::continuable_base::operator && operator&&\endlink  - connects another cti::continuable_base with an *all* logic.
  - \link cti::continuable_base::operator|| operator||\endlink  - connects another cti::continuable_base with an *any* logic.
  - \link cti::continuable_base::release release\endlink  - releases the cti::continuable_base and prevents the automatic invocation on destruction.
- Helper functions
  - \link cti::make_continuable make_continuable\endlink - creates a cti::continuable_base from a callback tanking function.
  - \link cti::all_of all_of\endlink - connects all given cti::continuable_base objects with an *all* logic.
  - \link cti::any_of any_of\endlink - connects all given cti::continuable_base objects with an *any* logic.

