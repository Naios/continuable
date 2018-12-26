add_executable(test-link
  ${CMAKE_CURRENT_LIST_DIR}/test-link.cpp)

target_link_libraries(test-link
  PRIVATE
    continuable)

add_test(NAME continuable-link-tests
  COMMAND test-link)
