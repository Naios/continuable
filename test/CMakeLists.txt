add_executable(continuable-tests
  ${CMAKE_CURRENT_LIST_DIR}/test-chaining.cpp)

target_link_libraries(continuable-tests
  PRIVATE
    continuable
    gtest-main)

add_test(NAME continuable-unit-tests COMMAND continuable-tests)
