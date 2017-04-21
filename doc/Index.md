![](https://raw.githubusercontent.com/Naios/continuable/master/doc/slideshow.gif)

This documentation covers the continuable library in detail

## Content

- **Class cti::continuable_base**  - main class for continuation chaining.
  - \link cti::continuable_base::then then\endlink - adds a callback or  cti::continuable_base to the invocation chain.
  - \link cti::continuable_base::operator && operator&&\endlink - connects another cti::continuable_base with an *all* logic.
  - \link cti::continuable_base::operator|| operator||\endlink - connects another cti::continuable_base with an *any* logic.
  - \link cti::continuable_base::operator>> operator>>\endlink - connects another cti::continuable_base with a *sequential* logic.
  - \link cti::continuable_base::futurize futurize\endlink - \copybrief cti::continuable_base::futurize
  - \link cti::continuable_base::done done\endlink - \copybrief cti::continuable_base::done
  - \link cti::continuable_base::freeze freeze \endlink - prevents the automatic invocation on destruction of the cti::continuable_base.
  - \link cti::continuable_base::is_frozen is_frozen\endlink - \copybrief cti::continuable_base::is_frozen

- **Helper functions**
  - \link cti::make_continuable make_continuable\endlink - creates a cti::continuable_base from a callback tanking function.
  - \link cti::when_all when_all\endlink - connects all given cti::continuable_base objects with an *all* logic.
  - \link cti::when_any when_any\endlink - connects all given cti::continuable_base objects with an *any* logic.
  - \link cti::when_seq when_seq\endlink - connects all given cti::continuable_base objects with a *sequence* logic.

- **Class cti::continuable_trait** - A trait class for defining your own cti::continuable_base trait with the type-erasure backend of your choice.
  - \link cti::continuable_trait::callback callback\endlink - \copybrief cti::continuable_trait::callback
  - \link cti::continuable_trait::continuation continuation\endlink - \copybrief cti::continuable_trait::continuation
  - \link cti::continuable_trait::continuable continuable\endlink - \copybrief cti::continuable_trait::continuable

- **GTest macros:**
  - \link ASSERT_ASYNC_COMPLETION ASSERT_ASYNC_COMPLETION \endlink - \copybrief ASSERT_ASYNC_COMPLETION
  - \link ASSERT_ASYNC_NEVER_COMPLETED ASSERT_ASYNC_NEVER_COMPLETED \endlink - \copybrief ASSERT_ASYNC_NEVER_COMPLETED
  - \link ASSERT_ASYNC_VALIDATION ASSERT_ASYNC_VALIDATION \endlink - \copybrief ASSERT_ASYNC_VALIDATION
  - \link ASSERT_ASYNC_BINARY_VALIDATION ASSERT_ASYNC_BINARY_VALIDATION \endlink - \copybrief ASSERT_ASYNC_BINARY_VALIDATION
  - \link EXPECT_ASYNC_RESULT EXPECT_ASYNC_RESULT \endlink - \copybrief EXPECT_ASYNC_RESULT
  - \link ASSERT_ASYNC_RESULT ASSERT_ASYNC_RESULT \endlink - \copybrief ASSERT_ASYNC_RESULT
  - \link ASSERT_ASYNC_TYPES ASSERT_ASYNC_TYPES \endlink - \copybrief ASSERT_ASYNC_TYPES
