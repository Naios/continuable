add_library(asio STATIC
  ${CMAKE_CURRENT_LIST_DIR}/asio/asio/src/asio.cpp)

target_include_directories(asio
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include)

target_compile_definitions(asio
  PUBLIC
    -DASIO_STANDALONE=1
    -DASIO_SEPARATE_COMPILATION=1
    -DASIO_NO_TYPEID=1)

if(WIN32)
target_compile_definitions(asio
  PUBLIC
    -D_WIN32_WINNT=0x0501)
endif()

target_compile_features(asio
  PUBLIC
    cxx_alias_templates
    cxx_auto_type
    cxx_decltype
    cxx_final
    cxx_lambdas
    cxx_variadic_templates
    cxx_defaulted_functions
    cxx_nullptr)
