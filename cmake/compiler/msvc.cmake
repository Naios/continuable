if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(PLATFORM 64)
else()
  set(PLATFORM 32)
endif()

if (PLATFORM EQUAL 64)
  target_compile_definitions(continuable-features-flags
    INTERFACE
      -D_WIN64)
endif()

target_compile_options(continuable-features-flags
  INTERFACE
    /MP2
    /bigobj)

target_compile_options(continuable-features-warnings
  INTERFACE
    /W4)

if (CTI_CONTINUABLE_WITH_NO_EXCEPTIONS)
  target_compile_definitions(continuable-features-noexcept
    INTERFACE
      -D_HAS_EXCEPTIONS=0)

  string(REGEX REPLACE "/GX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  message(STATUS "MSVC: Disabled exceptions")
endif()
