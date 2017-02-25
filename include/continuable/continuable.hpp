
/**
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

#ifndef CONTINUABLE_HPP_INCLUDED__
#define CONTINUABLE_HPP_INCLUDED__

#include <type_traits>

#include "continuable/continuable-base.hpp"
#include "function2/function2.hpp"

namespace cti {
/// Defines a copyable continuation type which uses the
/// function2 backend for type erasure.
///
/// Usable like: continuable<int, float>
template <typename... Args>
using continuable = continuable_of_t<
    continuable_erasure_of_t<fu2::function, fu2::unique_function, Args...>,
    Args...>;

/// Defines a non-copyable continuation type which uses the
/// function2 backend for type erasure.
///
/// Usable like: unique_continuable<int, float>
template <typename... Args>
using unique_continuable =
    continuable_of_t<continuable_erasure_of_t<fu2::unique_function,
                                              fu2::unique_function, Args...>,
                     Args...>;
} // end namespace cti

#endif // CONTINUABLE_HPP_INCLUDED__
