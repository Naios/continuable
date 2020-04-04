
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.0.0

  Copyright(c) 2015 - 2020 Denis Blank <denis.blank at outlook dot com>

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef CONTINUABLE_DETAIL_FEATURES_HPP_INCLUDED
#define CONTINUABLE_DETAIL_FEATURES_HPP_INCLUDED

// Defines CONTINUABLE_WITH_NO_EXCEPTIONS when exception support is disabled
#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS
#if defined(_MSC_VER)
#if !defined(_HAS_EXCEPTIONS) || (_HAS_EXCEPTIONS == 0)
#define CONTINUABLE_WITH_NO_EXCEPTIONS
#endif
#elif defined(__clang__)
#if !(__EXCEPTIONS && __has_feature(cxx_exceptions))
#define CONTINUABLE_WITH_NO_EXCEPTIONS
#endif
#elif defined(__GNUC__)
#if !__EXCEPTIONS
#define CONTINUABLE_WITH_NO_EXCEPTIONS
#endif
#endif
#endif // CONTINUABLE_WITH_NO_EXCEPTIONS

// clang-format off
// Detect if the whole standard is available
#if (defined(_MSC_VER) && defined(_HAS_CXX17) && _HAS_CXX17) ||                \
    (__cplusplus >= 201703L)
  #define CONTINUABLE_HAS_CXX17_CONSTEXPR_IF
  #define CONTINUABLE_HAS_CXX17_DISJUNCTION
  #define CONTINUABLE_HAS_CXX17_CONJUNCTION
  #define CONTINUABLE_HAS_CXX17_VOID_T
#else
  // Generic feature detection based on __has_feature
  // and other preprocessor definitions based on:
  // http://en.cppreference.com/w/User:D41D8CD98F/feature_testing_macros
  #if defined(__has_feature)
    #if !defined(CONTINUABLE_HAS_CXX17_CONSTEXPR_IF) &&                        \
        __has_feature(cxx_if_constexpr)
      #define CONTINUABLE_HAS_CXX17_CONSTEXPR_IF
    #endif
  #endif

  #if !defined(CONTINUABLE_HAS_CXX17_DISJUNCTION) &&                           \
      defined(__cpp_lib_experimental_logical_traits) &&                        \
      (__cpp_lib_experimental_logical_traits >= 201511)
    #define CONTINUABLE_HAS_CXX17_DISJUNCTION
  #endif

  #if !defined(CONTINUABLE_HAS_CXX17_CONJUNCTION) &&                           \
      defined(__cpp_lib_experimental_logical_traits) &&                        \
      (__cpp_lib_experimental_logical_traits >= 201511)
    #define CONTINUABLE_HAS_CXX17_CONJUNCTION
  #endif

  #if !defined(CONTINUABLE_HAS_CXX17_VOID_T) &&                                \
      defined(__cpp_lib_void_t) &&                                             \
      (__cpp_lib_void_t >= 201411)
    #define CONTINUABLE_HAS_CXX17_VOID_T
  #endif
#endif

/// Usually this is enabled by the CMake project
#if !defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)
  /// Define CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE when
  /// CONTINUABLE_WITH_EXPERIMENTAL_COROUTINE is defined.
  #if defined(CONTINUABLE_WITH_EXPERIMENTAL_COROUTINE)
    #define CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
  #elif defined(_MSC_VER)
    #if _MSC_FULL_VER >= 190023506
      #if defined(_RESUMABLE_FUNCTIONS_SUPPORTED)
        #define CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
      #endif // defined(_RESUMABLE_FUNCTIONS_SUPPORTED)
    #endif // _MSC_FULL_VER >= 190023506
  #elif defined(__clang__)
    #if defined(__cpp_coroutines) && (__cpp_coroutines >= 201707)
      #define CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE
    #endif // defined(__cpp_coroutines) && (__cpp_coroutines >= 201707)
  #endif // defined(__clang__)
#endif // !defined(CONTINUABLE_HAS_EXPERIMENTAL_COROUTINE)

/// Define CONTINUABLE_HAS_EXCEPTIONS when exceptions are used
#if !defined(CONTINUABLE_WITH_CUSTOM_ERROR_TYPE) &&                            \
    !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
  #define CONTINUABLE_HAS_EXCEPTIONS 1
#else
  #undef CONTINUABLE_HAS_EXCEPTIONS
#endif

/// Define CONTINUABLE_HAS_IMMEDIATE_TYPES when either
/// - CONTINUABLE_WITH_IMMEDIATE_TYPES is defined
/// - Building in release mode (NDEBUG is defined)
///
/// Build error messages will become more readable in debug mode while
/// we don't suffer any runtime penalty in release.
#if defined(CONTINUABLE_WITH_IMMEDIATE_TYPES) || defined(NDEBUG)
  #define CONTINUABLE_HAS_IMMEDIATE_TYPES 1
#else
  #undef CONTINUABLE_HAS_IMMEDIATE_TYPES
#endif
// clang-format on

#endif // CONTINUABLE_DETAIL_FEATURES_HPP_INCLUDED
