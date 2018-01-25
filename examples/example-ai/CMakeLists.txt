add_executable(example-ai
  ${CMAKE_CURRENT_LIST_DIR}/example-ai.cpp)

target_include_directories(example-ai
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(example-ai
  PRIVATE
    continuable)

target_compile_definitions(example-asio
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)
