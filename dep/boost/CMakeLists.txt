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

find_package(Boost 1.68 REQUIRED
  COMPONENTS
    system
    iostreams
    thread)

if (${Boost_FOUND})
  add_library(boost INTERFACE)

  target_link_libraries(boost
    INTERFACE
      Boost::system
      Boost::iostreams
      Boost::thread)

  target_compile_definitions(boost
    INTERFACE
      BOOST_ALL_NO_LIB
      BOOST_ASIO_DISABLE_BOOST_DATE_TIME
      BOOST_ASIO_DISABLE_BOOST_REGEX
      BOOST_RANGE_ENABLE_CONCEPT_ASSERT=0
      BOOST_THREAD_PROVIDES_FUTURE
      BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
      BOOST_FILESYSTEM_NO_DEPRECATED
      BOOST_THREAD_VERSION=4)
endif()
