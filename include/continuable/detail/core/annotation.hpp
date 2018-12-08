
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

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

#ifndef CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED
#define CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED

#include <type_traits>
#include <continuable/detail/core/types.hpp>
#include <continuable/detail/utility/traits.hpp>

namespace cti {
namespace detail {
template <typename Annotation>
struct annotation_trait;

/// Specialization for a present signature hint
template <typename... Args>
struct annotation_trait<traits::identity<Args...>> {
  template <typename Continuable>
  struct annotation_base {
    Continuable&& finish() {
      return std::move(*static_cast<Continuable*>(this));
    }
  };
};

namespace hints {
/// Extracts the signature we pass to the internal continuable
/// from an argument pack as specified by make_continuable.
///
/// This is the overload taking an arbitrary amount of args
template <typename... HintArgs>
constexpr auto from_explicit(traits::identity<HintArgs...> hint) {
  return hint;
}
/// \copybrief from_explicit
///
/// This is the overload taking a void arg.
constexpr auto from_explicit(traits::identity<void> /*hint*/) {
  return traits::identity<>{};
}
} // namespace hints
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_ANNOTATION_HPP_INCLUDED
