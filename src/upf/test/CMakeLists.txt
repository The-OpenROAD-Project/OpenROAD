or_integration_tests(
  "upf"
  TESTS
    levelshifter
    write
    isolation
    isolation_select
    supply
    supply_hier
)

if(ENABLE_TESTS)
  add_subdirectory(cpp)
endif()
