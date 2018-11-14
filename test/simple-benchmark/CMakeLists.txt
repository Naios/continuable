if (NOT CTI_CONTINUABLE_WITH_BENCHMARKS)
  return()
endif()

add_executable(benchmark-simple
  ${CMAKE_CURRENT_LIST_DIR}/benchmark-simple.cpp)

target_link_libraries(benchmark-simple
  PRIVATE
    benchmark
    benchmark_main
    boost
    continuable
    continuable-features-flags
    continuable-features-warnings
    continuable-features-noexcept)
