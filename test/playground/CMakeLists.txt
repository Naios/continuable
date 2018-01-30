set(LIB_SOURCES
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-types.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-trait.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-promise-base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-transforms.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/continuable-testing.hpp)
set(LIB_SOURCES_DETAIL
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/awaiting.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/base.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/composition.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/expected.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/hints.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/container-category.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/pack-traversal.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/pack-traversal-async.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/features.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/traits.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/transforms.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/types.hpp
  ${CMAKE_SOURCE_DIR}/include/continuable/detail/testing.hpp
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
    gtest
    continuable)

target_compile_definitions(test-playground
  PUBLIC
    -DCONTINUABLE_WITH_NO_EXCEPTIONS)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
