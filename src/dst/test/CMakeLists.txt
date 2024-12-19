or_integration_tests(
  "dst"
  # PASSFAIL_TESTS
    # cpp_tests
)

if(ENABLE_TESTS)
  add_subdirectory(cpp)
endif()
