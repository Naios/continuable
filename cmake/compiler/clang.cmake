# Enable full warnings
target_compile_options(continuable-features-warnings
  INTERFACE
    -Wall
    -pedantic
    -Wextra)

if (CTI_CONTINUABLE_WITH_NO_EXCEPTIONS)
  target_compile_options(continuable-features-noexcept
    INTERFACE
      -fno-exceptions)

  message(STATUS "Clang: Disabled exceptions")
endif()
