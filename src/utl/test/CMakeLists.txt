include("openroad")

set(TEST_NAMES
    test_info
    test_error
    test_suppress_message
    test_metrics
    logger_max_messages
    logger_redirection
    logger_redirection_nonewline
    tee
    tee_fails
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("utl" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
