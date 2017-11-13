cmake_minimum_required(VERSION 3.2)
project(continuable VERSION 2.0.0 LANGUAGES C CXX)

string(COMPARE EQUAL ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}
                     IS_TOP_LEVEL_PROJECT)

option(CTI_CONTINUABLE_WITH_TESTS
       "Build the continuable unit tests"
       ${IS_TOP_LEVEL_PROJECT})

option(CTI_CONTINUABLE_WITH_EXAMPLES
       "Build the continuable examples"
       ${IS_TOP_LEVEL_PROJECT})

option(CTI_CONTINUABLE_WITH_NO_EXCEPTIONS
       "Disable exception support"
       OFF)

option(CTI_CONTINUABLE_WITH_AWAIT
       "Enable co_await support"
       OFF)

include(cmake/CMakeLists.txt)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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
    Threads::Threads
    continuable-coroutines)

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
add_library(continuable::continuable ALIAS continuable)

target_link_libraries(continuable
  INTERFACE
    continuable-base
    function2)

# Create an install target
install(TARGETS continuable-coroutines continuable-base continuable
        EXPORT continuable-config
        INCLUDES DESTINATION include)

install(EXPORT continuable-config
        FILE continuable-config.cmake
        NAMESPACE continuable::
        DESTINATION share/continuable/cmake)

install(DIRECTORY include/continuable
        DESTINATION include FILES_MATCHING PATTERN "*.hpp")

install(FILES LICENSE.txt DESTINATION . RENAME continuable-LICENSE.txt)
install(FILES Readme.md DESTINATION . RENAME continuable-Readme.md)

# Setup CPack for bundling
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

include(CPack)

# Testing and examples
if (CTI_CONTINUABLE_WITH_TESTS OR CTI_CONTINUABLE_WITH_EXAMPLES)
  if (MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()

  enable_testing()

  add_subdirectory(doc)

  if (CTI_CONTINUABLE_WITH_TESTS)
    add_subdirectory(test)
  endif()

  if (CTI_CONTINUABLE_WITH_EXAMPLES)
    add_subdirectory(examples)
  endif()
endif ()
