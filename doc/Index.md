# Documentation of continuable

This documentation covers the continuable library in detail

## Content

- Class cti::continuable_base  - main class for continuation chaining
  - \link cti::continuable_base::then then\endlink - adds a callback to the chain
  - \link cti::continuable_base::operator && operator&&\endlink  - connects another continuable with an *all* logic.
  - \link cti::continuable_base::operator|| operator||\endlink  - connects another continuable with an *any* logic.
- Helper functions
  - \link cti::make_continuable make_continuable\endlink - creates a continuable_base from a callback tanking function.
  - \link cti::all_of all_of\endlink - connects all given continuables with an *all* logic.
  - \link cti::any_of any_of\endlink - connects all given continuables with an *any* logic.

