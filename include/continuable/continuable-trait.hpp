
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

#ifndef CONTINUABLE_TRAIT_HPP_INCLUDED
#define CONTINUABLE_TRAIT_HPP_INCLUDED

#include <cstddef>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-base.hpp>
#include <continuable/continuable-promise-base.hpp>
#include <continuable/detail/core/annotation.hpp>
#include <continuable/detail/core/types.hpp>

namespace cti {
/// \defgroup Types Types
/// provides the \link cti::continuable continuable\endlink and \link
/// cti::promise promise\endlink facility for type erasure.
/// \{

/// Trait to retrieve a continuable_base type with a given type-erasure backend.
///
/// Every object may me used as type-erasure backend as long as the
/// requirements of a `std::function` like wrapper are satisfied.
///
/// \tparam CallbackWrapper The type which is used to erase the callback.
///
/// \tparam ContinuationWrapper The type which is used to erase the
///         continuation data.
///
/// \tparam Args The current signature of the continuable.
template <template <std::size_t, typename...> class CallbackWrapper,
          template <std::size_t, typename...> class ContinuationWrapper,
          typename... Args>
class continuable_trait {

  using callback = CallbackWrapper<0U, void(Args...)&&,
                                   void(exception_arg_t,
                                        exception_t) &&>;

public:
  /// The promise type which is used to resolve continuations
  using promise =
      promise_base<callback, detail::traits::identity<Args...>>;

  /// The continuable type for the given parameters.
  using continuable =
      continuable_base<ContinuationWrapper<sizeof(callback), void(promise)>,
                       detail::traits::identity<Args...>>;
};
/// \}
} // namespace cti

#endif // CONTINUABLE_TRAIT_HPP_INCLUDED
