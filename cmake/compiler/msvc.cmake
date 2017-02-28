if (${MSVC_VERSION} LESS 1900)
  message(FATAL_ERROR "You are using an unsupported version of Visual Studio "
                      "which doesn't support all required C++11 features. "
                      "(Visual Studio 2015 (version >= 1900) is required!)")
endif()

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(PLATFORM 64)
else()
  set(PLATFORM 32)
endif()

if (PLATFORM EQUAL 64)
  add_definitions("-D_WIN64")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /MP /bigobj")

if (TESTS_NO_EXCEPTIONS)
  add_definitions(-D_HAS_EXCEPTIONS=0)
  string(REGEX REPLACE "/GX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  message(STATUS "MSVC: Disabled exceptions")
endif()
