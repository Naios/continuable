add_library(asio-example-deps INTERFACE)

target_include_directories(asio-example-deps
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(asio-example-deps
  INTERFACE
    asio
    continuable)

add_executable(example-asio
    ${CMAKE_CURRENT_LIST_DIR}/example-asio.cpp)

target_link_libraries(example-asio
  PRIVATE
    asio-example-deps)

target_compile_definitions(example-asio
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)
