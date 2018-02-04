if(WIN32)
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PLATFORM 64)
  else()
    set(PLATFORM 32)
  endif()

  if(DEFINED ENV{BOOST_ROOT})
    set(BOOST_ROOT $ENV{BOOST_ROOT})
    set(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib${PLATFORM}-msvc-14.1)
  endif()

  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
endif()

find_package(Boost 1.66
  COMPONENTS
    system
    iostreams)

if (${Boost_FOUND})
  add_library(boost INTERFACE)

  target_link_libraries(boost
    INTERFACE
      Boost::system
      Boost::iostreams)
      
  target_compile_definitions(boost
    INTERFACE
      -DBOOST_DATE_TIME_NO_LIB
      -DBOOST_REGEX_NO_LIB
      -DBOOST_CHRONO_NO_LIB)
endif()
