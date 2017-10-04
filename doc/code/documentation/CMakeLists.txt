add_executable(doc-documentation
  ${CMAKE_CURRENT_LIST_DIR}/doc-documentation.cpp)
target_link_libraries(doc-documentation
  PRIVATE
    continuable)
