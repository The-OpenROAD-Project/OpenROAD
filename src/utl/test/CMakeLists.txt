or_integration_tests(
  "utl"
  TESTS
    logger_max_messages
    logger_redirection
    logger_redirection_nonewline
    tee
    tee_fails
    test_error
    test_info
    test_metrics
    test_suppress_message
  PASSFAIL_TESTS
    cpp_tests
)

# Skipped
#test_error_exception
#utl_man_tcl_check
#utl_readme_msgs_check

add_subdirectory(cpp)
