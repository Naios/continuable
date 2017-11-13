foreach(STEP RANGE 4)
  set(PROJECT_NAME test-continuable-${STEP})
  set(TEST_NAME continuable-unit-tests-${STEP})

  add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable.hpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-await.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base-chaining.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base-destruct.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base-errors.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-base-partial.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-all.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-any.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-connection-seq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-erasure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-regression.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test-continuable-transforms.cpp)

  target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR})

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      gtest-main
      cxx_function
      continuable
      continuable-features-flags
      continuable-features-warnings
      continuable-features-noexcept)

  target_compile_definitions(${PROJECT_NAME}
    PUBLIC
      -DUNIT_TEST_STEP=${STEP})

  add_test(
    NAME ${TEST_NAME}
    COMMAND ${PROJECT_NAME})
endforeach()
