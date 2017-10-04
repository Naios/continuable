cmake_minimum_required(VERSION 3.2)
project(continuable-standalone LANGUAGES C CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(function2 REQUIRED)
find_package(continuable REQUIRED)

add_executable(continuable-standalone
  ${CMAKE_CURRENT_LIST_DIR}/continuable-standalone.cpp)

target_link_libraries(continuable-standalone
  PUBLIC
    continuable::continuable)
