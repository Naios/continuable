set(LIB_SOURCES
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-promise-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-testing.hpp)
set(LIB_SOURCES_DETAIL
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/composition.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/api.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/hints.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/features.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/traits.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/transforms.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/util.hpp)
set(TEST
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

add_executable(test-playground
  ${LIB_SOURCES}
  ${LIB_SOURCES_DETAIL}
  ${TEST})

source_group(src FILES ${LIB_SOURCES})
source_group(src\\detail FILES ${LIB_SOURCES_DETAIL})
source_group(test FILES ${TEST})

target_link_libraries(test-playground
  PRIVATE
    continuable)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
