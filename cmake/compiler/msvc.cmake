if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(PLATFORM 64)
else()
  set(PLATFORM 32)
endif()

if (PLATFORM EQUAL 64)
  add_definitions("-D_WIN64")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /MP")
