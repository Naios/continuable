if (NOT ${Boost_FOUND})
  message(STATUS "Boost not found, skipping boost examples")
  return()
endif()

add_executable(example-boost
    ${CMAKE_CURRENT_LIST_DIR}/server.hpp
    ${CMAKE_CURRENT_LIST_DIR}/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/example-boost.cpp)

target_include_directories(example-boost
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(example-boost
  PRIVATE
    continuable
    boost)

target_compile_definitions(example-boost
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)

