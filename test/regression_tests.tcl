# Flow tests only check the last line in the log (pass/fail).
# Ordered by instance count.
record_flow_tests {
  gcd_nangate45
  gcd_sky130hd
  gcd_sky130hs
  gcd_asap7

  ibex_sky130hd
  ibex_sky130hs

  aes_nangate45
  aes_sky130hd
  aes_sky130hs
  aes_asap7

  tinyRocket_nangate45

  jpeg_sky130hs
  jpeg_sky130hd
}

# Database loading tests for -db flag functionality
record_test open_db $test_dir "compare_logfile" "-db gcd_sky130hd.odb"
# For invalid DB, allow non-zero exit but require log to match ok
record_test open_db_invalid $test_dir "compare_logfile_allow_error" "-db nonexistent_file.odb"
