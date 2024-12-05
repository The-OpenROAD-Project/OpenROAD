include("openroad")

set(TEST_NAMES
    macro_only
    no_unfixed_macros
)

# Skipped
#mpl2_man_tcl_check
#mpl2_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("mpl2" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
