add_library(test-continuable-base STATIC
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable.hpp
  ${CMAKE_CURRENT_LIST_DIR}/test-continuable.cpp)

target_include_directories(test-continuable-base
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(test-continuable-base
  PUBLIC
    gtest-main
    cxx_function
    continuable
    continuable-features-flags
    continuable-features-warnings
    continuable-features-noexcept)

add_executable(test-continuable-single
  ${CMAKE_CURRENT_LIST_DIR}/single/test-continuable-connection-noinst
  ${CMAKE_CURRENT_LIST_DIR}/single/test-continuable-flat-variant.cpp
  ${CMAKE_CURRENT_LIST_DIR}/single/test-continuable-expected.cpp
  ${CMAKE_CURRENT_LIST_DIR}/single/test-continuable-traverse.cpp
  ${CMAKE_CURRENT_LIST_DIR}/single/test-continuable-traverse-async.cpp)

target_link_libraries(test-continuable-single
  PUBLIC
    test-continuable-base)

target_include_directories(test-continuable-single
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/single)

add_test(
  NAME continuable-unit-tests-single
  COMMAND test-continuable-single)

if (CTI_CONTINUABLE_WITH_LIGHT_TESTS)
  set(STEP_RANGE 0)
else()
  set(STEP_RANGE 4)
endif()

foreach(STEP RANGE ${STEP_RANGE})
  set(PROJECT_NAME test-continuable-multi-${STEP})
  set(TEST_NAME continuable-unit-tests-multi-${STEP})

  add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-await.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-base-chaining.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-base-destruct.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-base-errors.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-base-partial.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-all-seq-ag-1.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-all-seq-ag-2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-all-seq-op.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-all.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-any.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-connection-seq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-erasure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-regression.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multi/test-continuable-transforms.cpp)

  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      test-continuable-base)

  target_include_directories(${PROJECT_NAME}
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/multi)

  target_compile_definitions(${PROJECT_NAME}
    PUBLIC
      -DUNIT_TEST_STEP=${STEP})

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${PROJECT_NAME})
endforeach()
