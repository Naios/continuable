if(CONTINUABLE_UNIT_TESTS AND NOT TARGET gtest)
  add_subdirectory(googletest)
endif()

if(CONTINUABLE_UNIT_TESTS AND NOT TARGET cxx_function)
  add_subdirectory(cxx_function)
endif()

if(CONTINUABLE_UNIT_TESTS AND NOT TARGET asio)
  add_subdirectory(asio)
endif()

if(NOT TARGET function2)
  add_subdirectory(function2)
endif()
