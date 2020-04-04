add_library(asio STATIC
  ${CMAKE_CURRENT_LIST_DIR}/asio/asio/src/asio.cpp
  ${CMAKE_CURRENT_LIST_DIR}/include/boost/throw_exception.hpp)

target_include_directories(asio
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include
    ${CMAKE_CURRENT_LIST_DIR}/include)

target_compile_definitions(asio
  PUBLIC
    -DASIO_STANDALONE=1
    -DASIO_SEPARATE_COMPILATION=1
    -DASIO_NO_TYPEID=1)

if (CTI_CONTINUABLE_WITH_NO_EXCEPTIONS)
  target_compile_definitions(asio
    PUBLIC
      -DASIO_NO_EXCEPTIONS=1
      -DASIO_HAS_BOOST_THROW_EXCEPTION=1)

  message(STATUS "ASIO: Disabled exceptions")
endif()

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
