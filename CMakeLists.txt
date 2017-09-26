cmake_minimum_required(VERSION 3.2)
project(continuable C CXX)

# Dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CONTINUABLE_UNIT_TESTS ON)
endif()

add_subdirectory(dep)

# continuable-base
add_library(continuable-base INTERFACE)

target_include_directories(continuable-base
  INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/include")

target_link_libraries(continuable-base
  INTERFACE
    Threads::Threads)

target_compile_features(continuable-base
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
    cxx_trailing_return_types
    cxx_return_type_deduction
    $<$<COMPILE_FEATURES:cxx_std_17>:cxx_std_17>)

add_library(continuable INTERFACE)

target_link_libraries(continuable
  INTERFACE
    continuable-base
    function2)

# Testing
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if (MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()

  enable_testing()

  include(cmake/CMakeLists.txt)


  add_subdirectory(examples)
  add_subdirectory(test)
endif ()
