
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

#ifndef CONTINUABLE_HPP_INCLUDED
#define CONTINUABLE_HPP_INCLUDED

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
namespace cti {}

#include <continuable/continuable-base.hpp>
#include <continuable/continuable-connections.hpp>
#include <continuable/continuable-coroutine.hpp>
#include <continuable/continuable-operations.hpp>
#include <continuable/continuable-primitives.hpp>
#include <continuable/continuable-promise-base.hpp>
#include <continuable/continuable-promisify.hpp>
#include <continuable/continuable-result.hpp>
#include <continuable/continuable-transforms.hpp>
#include <continuable/continuable-traverse-async.hpp>
#include <continuable/continuable-traverse.hpp>
#include <continuable/continuable-types.hpp>

#endif // CONTINUABLE_HPP_INCLUDED
