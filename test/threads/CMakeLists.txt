add_executable(test-threads
  ${CMAKE_CURRENT_LIST_DIR}/test-threads.cpp)

target_link_libraries(test-threads
  PRIVATE
    continuable)

add_test(NAME continuable-threads-tests
  COMMAND test-threads)
