set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/continuable)

file(GLOB LIB_SOURCES CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.hpp)
file(GLOB LIB_SOURCES_DETAIL CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/*.hpp)
file(GLOB LIB_SOURCES_DETAIL_CONNECTIONS CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/connection/*.hpp)
file(GLOB LIB_SOURCES_DETAIL_CORE CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/core/*.hpp)
file(GLOB LIB_SOURCES_DETAIL_OTHER CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/other/*.hpp)
file(GLOB LIB_SOURCES_DETAIL_TRAVERSAL CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/traversal/*.hpp)
file(GLOB LIB_SOURCES_DETAIL_UTILITY CONFIGURE_DEPENDS ${INCLUDE_DIR}/detail/utility/*.hpp)

set(TEST
  ${CMAKE_CURRENT_LIST_DIR}/test-playground.cpp)

add_executable(test-playground
  ${LIB_SOURCES}
  ${LIB_SOURCES_DETAIL}
  ${LIB_SOURCES_DETAIL_CONNECTIONS}
  ${LIB_SOURCES_DETAIL_CORE}
  ${LIB_SOURCES_DETAIL_OTHER}
  ${LIB_SOURCES_DETAIL_TRAVERSAL}
  ${LIB_SOURCES_DETAIL_UTILITY}
  ${TEST})

source_group(continuable FILES ${LIB_SOURCES})
source_group(continuable\\detail FILES ${LIB_SOURCES_DETAIL})
source_group(continuable\\detail\\connections FILES ${LIB_SOURCES_DETAIL_CONNECTIONS})
source_group(continuable\\detail\\core FILES ${LIB_SOURCES_DETAIL_CORE})
source_group(continuable\\detail\\other FILES ${LIB_SOURCES_DETAIL_OTHER})
source_group(continuable\\detail\\traversal FILES ${LIB_SOURCES_DETAIL_TRAVERSAL})
source_group(continuable\\detail\\utility FILES ${LIB_SOURCES_DETAIL_UTILITY})
source_group(test FILES ${TEST})

target_link_libraries(test-playground
  PRIVATE
    gtest
    continuable
    continuable-features-flags
    continuable-features-warnings
    continuable-features-noexcept)

add_test(NAME continuable-playground-tests
  COMMAND test-playground)
