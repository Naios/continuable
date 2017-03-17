add_executable(test-mock
  ${CMAKE_CURRENT_LIST_DIR}/test-mock.cpp)

target_link_libraries(test-mock
  PRIVATE
    continuable)

add_test(NAME continuable-mock-tests
  COMMAND test-mock)
