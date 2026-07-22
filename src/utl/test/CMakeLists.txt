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

#test_error_exception

add_subdirectory(cpp)
