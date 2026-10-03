or_integration_tests(
  "upf"
  TESTS
    levelshifter
    write
    isolation
    isolation_select
    supply
)

if(ENABLE_TESTS)
  add_subdirectory(cpp)
endif()
