or_integration_tests(
  "upf"
  TESTS
    levelshifter
    write
    isolation
    isolation_select
)

if(ENABLE_TESTS)
  add_subdirectory(cpp)
endif()
