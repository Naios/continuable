add_executable(test-playground
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

target_link_libraries(test-playground
  PRIVATE
    continuable)
