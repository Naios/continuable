add_executable(example-slideshow
  ${CMAKE_CURRENT_LIST_DIR}/example-slideshow.cpp)
target_link_libraries(example-slideshow
  PRIVATE
    continuable)
