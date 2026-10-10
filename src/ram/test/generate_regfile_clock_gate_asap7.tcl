# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_clock_gate_asap7
# generate_regfile with a clock gate per word (write_style clock_gate):
# every read port reads every word, and a write opens the clock gate of
# the word it addresses, and only that one, with the write data on its
# flops' D. Checked by constant propagation through the generated cells.

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries
set spec [rf_spec generate_regfile_asap7.regfile \
  generate_regfile_clock_gate_asap7.regfile {"write_style clock_gate"}]
generate_regfile -spec $spec -check_ports generate_regfile_asap7_rtl.v

for { set w 1 } { $w < 8 } { incr w } {
  rf_store $w 4 [rf_word_value $w 4]
}
foreach port { r0 r1 } {
  for { set a 0 } { $a < 8 } { incr a } {
    rf_drive ${port}_addr 3 $a
    set expected [expr { $a == 0 ? 0 : [rf_word_value $a 4] }]
    check "$port reads word $a" { rf_read ${port}_data 4 } $expected
  }
}

proc check_writes { port other } {
  rf_drive_bit ${other}_en 0
  rf_drive_bit ${port}_en 1
  foreach a { 1 3 6 } {
    set data [expr { (~[rf_word_value $a 4]) & 15 }]
    rf_drive ${port}_addr 3 $a
    rf_drive ${port}_data 4 $data
    for { set w 1 } { $w < 8 } { incr w } {
      check "$port writing word $a: word $w's clock gate" \
        { rf_value w${w}_icg/ENA } [expr { $w == $a ? 1 : 0 }]
    }
    check "$port writing word $a: its flops load the data" { rf_next $a 4 } $data
  }
  rf_drive_bit ${port}_en 0
  for { set w 1 } { $w < 8 } { incr w } {
    check "no write: word $w's clock gate is shut" { rf_value w${w}_icg/ENA } 0
  }
}
check_writes w0 w1
check_writes w1 w0

exit_summary
