cmake_minimum_required(VERSION 3.2)
project(continuable CXX)

set(continuable_VERSION_MAJOR 1)

add_library(continuable INTERFACE)

target_include_directories(continuable
  INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/include")

target_compile_features(continuable
  INTERFACE
    cxx_alias_templates
    cxx_auto_type
    cxx_constexpr
    cxx_decltype
    cxx_decltype_auto
    cxx_final
    cxx_lambdas
    cxx_generic_lambdas
    cxx_variadic_templates
    cxx_defaulted_functions
    cxx_nullptr
    cxx_trailing_return_types)

# Set up the test environment for testing
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_executable(incubator
    "${CMAKE_CURRENT_LIST_DIR}/include/continuable/continuable.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/incubator.cpp")
  target_link_libraries(incubator continuable)

  if (MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()

  enable_testing()

  include(cmake/CMakeLists.txt)

  add_subdirectory(dep)
  add_subdirectory(test)
endif ()
