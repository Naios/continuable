add_executable(example-documentation
  ${CMAKE_CURRENT_LIST_DIR}/example-documentation.cpp)
target_link_libraries(example-documentation
  PRIVATE
    continuable)
