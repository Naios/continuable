add_executable(example-asio
    ${CMAKE_CURRENT_LIST_DIR}/example-asio.cpp)

target_include_directories(example-asio
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(example-asio
  PRIVATE
    asio
    continuable)

target_compile_definitions(example-asio
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)
