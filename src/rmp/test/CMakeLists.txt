or_integration_tests(
  "rmp"
  TESTS
    aes_nangate45
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
    aes_asap7
    gcd_asap7
    memory_nangate45
    aes_dontuse_nangate45
    aes_donttouch_nangate45
    aes_annealing
    gcd_annealing1
    gcd_annealing2
    aes_genetic
    gcd_genetic
)


if (ENABLE_TESTS)
    add_subdirectory(cpp)
endif()
