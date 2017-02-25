add_library(gtest STATIC
  ${CMAKE_CURRENT_LIST_DIR}/googletest/googletest/src/gtest-all.cc)

target_compile_features(gtest
  PUBLIC
    cxx_alias_templates
    cxx_auto_type
    cxx_decltype
    cxx_final
    cxx_lambdas
    cxx_variadic_templates
    cxx_defaulted_functions
    cxx_nullptr)

target_include_directories(gtest
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/googletest/googletest
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/googletest/googletest/include)

target_compile_definitions(gtest
  PUBLIC
    -DGTEST_HAS_PTHREAD=0
    -DGTEST_LANG_CXX11=1)

add_library(gtest-main STATIC
  ${CMAKE_CURRENT_LIST_DIR}/googletest/googletest/src/gtest_main.cc)

target_link_libraries(gtest-main
  PUBLIC
    gtest)

add_library(gmock STATIC
  ${CMAKE_CURRENT_LIST_DIR}/googletest/googlemock/src/gmock-all.cc)

target_link_libraries(gmock
  PUBLIC
    gtest)

target_include_directories(gmock
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/googletest/googlemock
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/googletest/googlemock/include)
