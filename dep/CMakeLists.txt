if(NOT TARGET function2)
  add_subdirectory(function2)
endif()

if (CTI_CONTINUABLE_WITH_TESTS OR CTI_CONTINUABLE_WITH_EXAMPLES)
  if(NOT TARGET gtest)
    add_subdirectory(googletest)
  endif()

  if(NOT TARGET asio)
    add_subdirectory(asio)
  endif()
endif()
