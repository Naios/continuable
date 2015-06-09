project(fluent++ C CXX)
cmake_minimum_required(VERSION 2.8)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(Boost 1.55 REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    add_definitions(-Wall -Wextra)
endif()

file(GLOB_RECURSE FLUENT_SOURCES fluent++/*.cpp fluent++/*.hpp fluent++/*.h)
add_library(fluent++ STATIC ${FLUENT_SOURCES})

include_directories(fluent++)

set(TEST_SOURCES test.cpp)

add_executable(fluent_test ${TEST_SOURCES})

target_link_libraries(fluent_test
  fluent++
)
