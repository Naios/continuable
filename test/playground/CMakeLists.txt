set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.hpp)

set(TEST
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

add_executable(test-playground
  ${LIB_SOURCES}
  ${TEST})

group_sources(${CMAKE_CURRENT_LIST_DIR}
              ${CMAKE_SOURCE_DIR}/include/)

target_link_libraries(test-playground
  PRIVATE
    gtest
    continuable
    continuable-features-flags
    continuable-features-warnings
    continuable-features-noexcept)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
