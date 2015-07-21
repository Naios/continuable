project(Continue.cpp C CXX)
cmake_minimum_required(VERSION 2.8)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

if(MSVC)
  if(${MSVC_VERSION} LESS 1900)
    message(FATAL_ERROR "You are using an unsupported version of Visual Studio "
                        "which doesn't support all required C++14 features. "
                        "(Visual Studio 2015 (Version >= 1900) is required!)")

    message(STATUS "bad")
  endif()

  if(PLATFORM EQUAL 64)
    add_definitions("-D_WIN64")
  else()
    # ...
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

else()
  # Check C++14 Compiler support.
  CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)

  if(NOT COMPILER_SUPPORTS_CXX14)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      # Clang > 3.4 is ok
    else()
      message(FATAL_ERROR "Your compiler has no C++14 capability!")
    endif()
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

  # Find Threads
  find_package (Threads)
endif()

find_package(Boost 1.55 REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

file(GLOB_RECURSE LIB_SOURCES include/*.cpp include/*.hpp include/*.h)
add_library(continue STATIC ${LIB_SOURCES})

include_directories(include dep/concurrentqueue)

set(TEST_SOURCES test.cpp)

add_executable(continue_test ${TEST_SOURCES})

target_link_libraries(continue_test
  continue
  ${CMAKE_THREAD_LIBS_INIT}
)
