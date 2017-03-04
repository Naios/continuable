if(CONTINUABLE_UNIT_TESTS AND NOT TARGET gtest)
  add_subdirectory(googletest)
endif()

if(NOT TARGET function2)
  add_subdirectory(function2)
endif()
