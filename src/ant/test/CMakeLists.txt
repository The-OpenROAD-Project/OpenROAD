or_integration_tests(
  "ant"
  TESTS
    check_grt1
  PASSFAIL_TESTS
    ant_api
    check_antennas_cmd
)

if(ENABLE_TESTS)
  add_subdirectory(cpp)
endif()
