add_executable(test-continuable
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-testing.hpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable.hpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-all.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-any.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-seq.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-erasure.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable-recursion.cpp)

target_link_libraries(test-continuable
  PRIVATE
    gtest-main
    continuable)

add_test(NAME continuable-unit-tests
  COMMAND test-continuable)
