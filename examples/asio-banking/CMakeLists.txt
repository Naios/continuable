add_executable(example-asio-banking
    ${CMAKE_CURRENT_LIST_DIR}/example-asio-banking.cpp)

target_include_directories(example-asio-banking
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(example-asio-banking
  PRIVATE
    asio
    continuable)
