or_integration_tests(
  "ram"
  TESTS
    # make_8x8_sky130 is bazel-only: DRT routing is FP-sensitive and the
    # golden .defok matches the clang build used by bazel, not the gcc
    # build typically used by cmake. See src/ram/test/BUILD.
    make_7x7_nangate45
  PASSFAIL_TESTS
    write_ports_nangate45
    and_or_read_asap7
    zero_word_asap7
    reset_asap7
    reset_nangate45
    banks_asap7
    banked_read_ports_asap7
    register_read_address_asap7
    write_mux_asap7
    periphery_unplaced_asap7
    cells_name_asap7
    decoder_word_asap7
    bit_folds_asap7
    periphery_verilog_asap7
    write_priority_or_asap7
    service_columns_asap7
    bank_column_layout_asap7
    option_errors_asap7
)

