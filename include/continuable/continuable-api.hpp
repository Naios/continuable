
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

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
*/

#ifndef CONTINUABLE_DETAIL_API_HPP_INCLUDED__
#define CONTINUABLE_DETAIL_API_HPP_INCLUDED__

/// Declares the continuable library namespace.
///
/// The most important class is cti::continuable_base, that provides the
/// whole functionality for continuation chaining.
///
/// The class cti::continuable_base is created through the
/// cti::make_continuable() function which accepts a callback taking function.
///
/// Also there are following support functions available:
/// - cti::when_all() - connects cti::continuable_base's to an `all` connection.
/// - cti::when_any() - connects cti::continuable_base's to an `any` connection.
/// - cti::when_seq() - connects cti::continuable_base's to a sequence.
///
namespace cti {
/// The main class of the continuable library, it provides the functionality
/// for chaining callbacks and continuations together to a unified hierarchy.
///
/// The most important method is the cti::continuable_base::then() method,
/// which allows to attach a callback to the continuable.
///
/// Use the continuable types defined in `continuable/continuable.hpp`,
/// in order to use this class.
///
/// \tparam Data The internal data which is used to store the current
///         continuation and intermediate lazy connection result.
///
/// \tparam Annotation The internal data used to store the current signature
///         hint or strategy used for combining lazy connections.
///
/// \note Nearly all methods of the cti::continuable_base are required to be
///       called as r-value. This is required because the continuable carries
///       variables which are consumed when the object is transformed as part
///       of a method call. You may copy a continuable which underlying
///       storages are copyable to split the call hierarchy into multiple parts.
///
/// \attention The continuable_base objects aren't intended to be stored.
///            If you want to store a continuble_base you should always
///            call the continuable_base::freeze method for disabling the
///            invocation on destruction.
///
/// \since version 1.0.0
template <typename Data, typename Annotation>
class continuable_base;

/// Declares the internal private namespace of the continuable library
/// which isn't intended to be used by users of the library.
namespace detail {}
} // namespace cti

#endif // CONTINUABLE_DETAIL_API_HPP_INCLUDED__
