# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_write_priority_asap7
# generate_regfile with `write_priority last`: when both write ports
# write one word in one cycle, the word loads the later port's data;
# writes to different words each load their own. Checked by constant
# propagation through the generated cells.

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries
set spec [rf_spec generate_regfile_asap7.regfile \
  generate_regfile_write_priority_asap7.regfile {"write_priority last"}]
generate_regfile -spec $spec -check_ports generate_regfile_asap7_rtl.v

for { set w 1 } { $w < 8 } { incr w } {
  rf_store $w 4 [rf_word_value $w 4]
}
rf_drive_bit w0_en 1
rf_drive_bit w1_en 1
foreach a { 1 4 7 } {
  rf_drive w0_addr 3 $a
  rf_drive w1_addr 3 $a
  rf_drive w0_data 4 5
  rf_drive w1_data 4 10
  check "both ports write word $a: it loads w1's data" { rf_next $a 4 } 10
}
rf_drive w0_addr 3 2
rf_drive w1_addr 3 6
check "w0 writes word 2: it loads w0's data" { rf_next 2 4 } 5
check "w1 writes word 6: it loads w1's data" { rf_next 6 4 } 10
check "word 3 holds" { rf_next 3 4 } [rf_word_value 3 4]

exit_summary
