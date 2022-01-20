
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v4.2.0

  Copyright(c) 2015 - 2022 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_TRANSFORMS_HPP_INCLUDED
#define CONTINUABLE_TRANSFORMS_HPP_INCLUDED

#include <continuable/transforms/wait.hpp>
#include <continuable/transforms/future.hpp>

namespace cti {
/// \defgroup Transforms Transforms
/// provides utilities to convert
/// \link continuable_base continuable_bases\endlink to other
/// types such as (`std::future`).
/// \{

/// The namespace transforms declares callable objects that transform
/// any continuable_base to an object or to a continuable_base itself.
///
/// Transforms can be applied to continuables through using
/// the cti::continuable_base::apply method accordingly.
namespace transforms {}
} // namespace cti

#endif // CONTINUABLE_TRANSFORMS_HPP_INCLUDED
