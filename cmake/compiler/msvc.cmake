
# Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(PLATFORM 64)
else()
  set(PLATFORM 32)
endif()

if (PLATFORM EQUAL 64)
  target_compile_definitions(continuable-features-flags
    INTERFACE
      -D_WIN64)
endif()

if (CTI_CONTINUABLE_WITH_CONCURRENT_JOBS)
  target_compile_options(continuable-features-flags
    INTERFACE
      /MP${CTI_CONTINUABLE_WITH_CONCURRENT_JOBS})
else()
  target_compile_options(continuable-features-flags
    INTERFACE
      /MP)
endif()

target_compile_options(continuable-features-flags
  INTERFACE
    /bigobj
    /permissive-)

target_compile_options(continuable-features-warnings
  INTERFACE
    /W4)

if (NOT CTI_CONTINUABLE_WITH_CPP_LATEST)
  target_compile_options(continuable-features-flags
    INTERFACE
      /std:c++14)
endif()

if (CTI_CONTINUABLE_WITH_NO_EXCEPTIONS)
  target_compile_definitions(continuable-features-noexcept
    INTERFACE
      -D_HAS_EXCEPTIONS=0)

  string(REGEX REPLACE "/GX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  message(STATUS "MSVC: Disabled exceptions")
endif()
