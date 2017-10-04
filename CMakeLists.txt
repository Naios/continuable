cmake_minimum_required(VERSION 3.2)
project(continuable VERSION 2.0.0 LANGUAGES C CXX)

# Dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (NOT DEFINED CONTINUABLE_UNIT_TESTS
    AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CONTINUABLE_UNIT_TESTS ON)
endif()

add_subdirectory(dep)

# continuable-base
add_library(continuable-base INTERFACE)
add_library(continuable::continuable-base ALIAS continuable-base)

target_include_directories(continuable-base
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include>)

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
    cxx_return_type_deduction)

add_library(continuable INTERFACE)

target_link_libraries(continuable
  INTERFACE
    continuable-base
    function2)

# Create an install target
install(TARGETS continuable-base
        EXPORT ContinuableConfig
        INCLUDES DESTINATION include)

install(EXPORT ContinuableConfig
        FILE ContinuableConfig.cmake
        NAMESPACE continuable::
        DESTINATION lib/cmake/continuable)

install(DIRECTORY include/continuable
        DESTINATION include FILES_MATCHING PATTERN "*.hpp")

install(FILES LICENSE Readme.md
        DESTINATION .)

# Setup CPack for bundling
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Since the header only library is platform independent
# we name the packages after the native line feed
if(WIN32)
  set(CPACK_SYSTEM_NAME "crlf")
else()
  set(CPACK_SYSTEM_NAME "lf")
endif()

include(CPack)

# Testing
if (CONTINUABLE_UNIT_TESTS)
  if (MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()

  enable_testing()

  include(cmake/CMakeLists.txt)

  add_subdirectory(doc)
  add_subdirectory(examples)
  add_subdirectory(test)
endif ()
