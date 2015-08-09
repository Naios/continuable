project(Continue.cpp C CXX)
cmake_minimum_required(VERSION 2.8)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

if(MSVC)
  if(${MSVC_VERSION} LESS 1800)
    message(FATAL_ERROR "You are using an unsupported version of Visual Studio "
                        "which doesn't support all required C++11 features. "
                        "(Visual Studio 2013 (Version >= 1800) is required!)")

  endif()

  if(PLATFORM EQUAL 64)
    add_definitions("-D_WIN64")
  else()
    # ...
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

else()
  # Check C++11 Compiler support.
  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

  if(NOT COMPILER_SUPPORTS_CXX11)
    # If not given try to enable C++1y
    CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)

    if(NOT COMPILER_SUPPORTS_CXX1Y)
      message(FATAL_ERROR "Your compiler has no C++11 capability!")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    endif()
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  # Enable full warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

  # Find Threads
  find_package (Threads)
endif()

find_package(Boost 1.55 REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

file(GLOB_RECURSE LIB_SOURCES include/*.cpp include/*.hpp include/*.h)
add_library(Continuable STATIC ${LIB_SOURCES})

include_directories(include testing)

file(GLOB TEST_SOURCES *.cpp *.hpp *.h)

add_executable(ContinuableTest ${TEST_SOURCES})

target_link_libraries(ContinuableTest
  Continuable
  ${CMAKE_THREAD_LIBS_INIT}
)
