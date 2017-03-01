foreach(STEP RANGE 5)
  set(PROJECT_NAME test-continuable-${STEP})
  set(TEST_NAME continuable-unit-tests-${STEP})

  add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable.hpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-all.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-any.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-seq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-erasure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-recursion.cpp)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      gtest-main
      continuable)

  target_compile_definitions(${PROJECT_NAME}
    PUBLIC
      -DUNIT_TEST_STEP=${STEP})

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${PROJECT_NAME})
endforeach()
