or_integration_tests(
  "rmp"
  TESTS
    blif_reader
    blif_reader_const
    blif_reader_sequential
    blif_writer
    blif_writer_consts
    blif_writer_hanging
    blif_writer_input_output
    blif_writer_sequential
    const_cell_removal
    gcd_restructure
)

# Skipped
#rmp_man_tcl_check
#rmp_readme_msgs_check

if (ENABLE_TESTS)
    add_subdirectory(cpp)
endif()
