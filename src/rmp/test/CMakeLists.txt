include("openroad")

set(TEST_NAMES
    gcd_restructure
    const_cell_removal
    blif_writer
    blif_writer_input_output
    blif_writer_consts
    blif_writer_hanging
    blif_writer_sequential
    blif_reader
    blif_reader_const
    blif_reader_sequential
)

# Skipped
#rmp_man_tcl_check
#rmp_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("rmp" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

if (ENABLE_TESTS)
    add_subdirectory(cpp)
endif()
