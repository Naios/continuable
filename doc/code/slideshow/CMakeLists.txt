add_executable(doc-slideshow
  ${CMAKE_CURRENT_LIST_DIR}/doc-slideshow.cpp)
target_link_libraries(doc-slideshow
  PRIVATE
    continuable)
