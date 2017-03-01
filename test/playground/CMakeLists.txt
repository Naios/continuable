add_executable(test-playground
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-testing.hpp
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

target_link_libraries(test-playground
  PRIVATE
    continuable)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
