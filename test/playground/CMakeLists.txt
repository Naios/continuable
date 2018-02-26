set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/continuable)

file(GLOB LIB_SOURCES ${INCLUDE_DIR}/*.hpp)
file(GLOB_RECURSE LIB_SOURCES_DETAIL ${INCLUDE_DIR}/detail/*.hpp)

set(TEST
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

add_executable(test-playground
  ${LIB_SOURCES}
  ${LIB_SOURCES_DETAIL}
  ${TEST})

source_group(continuable FILES ${LIB_SOURCES})
source_group(continuable\\detail FILES ${LIB_SOURCES_DETAIL})
source_group(test FILES ${TEST})

target_link_libraries(test-playground
  PRIVATE
    gtest
    continuable)

target_compile_definitions(test-playground
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
