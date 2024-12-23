function(or_integration_test_single tool_name test_name test_type check_log check_passfail)
  unset(TEST_FOUND PARENT_SCOPE)

  if (${test_type} STREQUAL "tcl")
    set(TEST_EXT tcl)
  elseif (${test_type} STREQUAL "python")
    set(TEST_EXT py)
  else()
    message(FATAL_ERROR "${test_type} is not supported by testing")
  endif()

  set(TEST_FILE "${CMAKE_CURRENT_LIST_DIR}/${test_name}.${TEST_EXT}")
  set(TEST_NAME "${tool_name}.${test_name}.${TEST_EXT}")

  if(EXISTS "${TEST_FILE}")
    add_test(
      NAME ${TEST_NAME}
      COMMAND ${BASH_PROGRAM} ${OpenROAD_SOURCE_DIR}/test/regression_test.sh
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

    string(
      CONCAT ENV
      "OPENROAD_EXE=$<TARGET_FILE:openroad>;"
      "TEST_NAME=${test_name};"
      "TEST_EXT=${TEST_EXT};"
      "TEST_TYPE=${test_type};"
      "TEST_CHECK_LOG=${check_log};"
      "TEST_CHECK_PASSFAIL=${check_passfail};"
    )

    set_property(
      TEST ${TEST_NAME}
      PROPERTY ENVIRONMENT ${ENV}
    )

    set(LABELS "IntegrationTest ${test_type} ${tool_name}")
    if(check_log STREQUAL "True")
      set(LABELS "${LABELS} log_compare")
    endif()
    if(check_passfail STREQUAL "True")
      set(LABELS "${LABELS} passfail")
    endif()

    set_tests_properties(
      ${TEST_NAME}
      PROPERTIES LABELS "${LABELS}"
    )
    set(TEST_FOUND TRUE PARENT_SCOPE)
  else()
    set(TEST_FOUND FALSE PARENT_SCOPE)
    # message(WARNING "Test ${TEST_FILE} is missing")
  endif()

endfunction()

function(or_integration_tests tool_name)

  # Parse args
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs TESTS PASSFAIL_TESTS)

  cmake_parse_arguments(
      ARG  # prefix on the parsed args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  if (DEFINED ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown argument(s) to or_integration_tests: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if (DEFINED ARG_KEYWORDS_MISSING_VALUES)
     message(FATAL_ERROR "Missing value for argument(s) to or_integration_tests: ${ARG_KEYWORDS_MISSING_VALUES}")
  endif()

  if (DEFINED ARG_TESTS)
    foreach(TEST_NAME IN LISTS ARG_TESTS)

      or_integration_test_single(${tool_name} ${TEST_NAME} tcl True False)
      set(tcl_found ${TEST_FOUND})
      or_integration_test_single(${tool_name} ${TEST_NAME} python True False)
      set(py_found ${TEST_FOUND})
      
      if(NOT tcl_found AND NOT py_found)
        message(FATAL_ERROR "Test ${TEST_NAME} is missing for ${tool_name}")
      endif()

    endforeach()
  endif()

  if (DEFINED ARG_PASSFAIL_TESTS)
    foreach(TEST_NAME IN LISTS ARG_PASSFAIL_TESTS)

      or_integration_test_single(${tool_name} ${TEST_NAME} tcl False True)
      set(tcl_found ${TEST_FOUND})
      or_integration_test_single(${tool_name} ${TEST_NAME} python False True)
      set(py_found ${TEST_FOUND})

      if(NOT tcl_found AND NOT py_found)
        message(FATAL_ERROR "Test ${TEST_NAME} is missing for ${tool_name}")
      endif()

    endforeach()
  endif()

endfunction()
