
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

#ifndef CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED

#include <type_traits>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
namespace hints {
/// Extracts the signature we pass to the internal continuable
/// from an argument pack as specified by make_continuable.
///
/// This is the overload taking an arbitrary amount of args
template <typename... HintArgs>
struct from_args : std::common_type<signature_arg_t<HintArgs...>> {};
template <>
struct from_args<void> : std::common_type<signature_arg_t<>> {};
} // namespace hints
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED
