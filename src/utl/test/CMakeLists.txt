include("openroad")

set(TEST_NAMES
    logger_max_messages
    logger_redirection
    logger_redirection_nonewline
    tee
    tee_fails
    test_error
    test_info
    test_metrics
    test_suppress_message
)

# Skipped
#test_error_exception
#utl_man_tcl_check
#utl_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("utl" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
