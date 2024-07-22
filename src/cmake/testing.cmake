function(or_integration_test tool_name test_name regression_binary)
  add_test (
    NAME ${tool_name}.${test_name}
    COMMAND ${BASH_PROGRAM} ${regression_binary} ${test_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )

  string(CONCAT ENV
      "TEST_TYPE=compare_logfile;"
      "CTEST_TESTNAME=${test_name};"
      "DIFF_LOCATION=${CMAKE_CURRENT_LIST_DIR}/results/${test_name}.diff"
  )

  set_property(TEST ${tool_name}.${test_name}
               PROPERTY ENVIRONMENT ${ENV})

  set_tests_properties(${tool_name}.${test_name}
                       PROPERTIES LABELS "IntegrationTest")
endfunction()
