# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_asap7
# generate_regfile builds the register file its spec describes: every
# read port reads every word, word 0 reads zero, a write port loads only
# the word it addresses and every other word holds, and the abstract and
# the timing model name every port. A spec whose ports are not the RTL
# module's is refused. Checked by constant propagation through the
# generated cells, not against a golden netlist.

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries

set failed [catch {
  tee -quiet -variable log {
    generate_regfile -spec generate_regfile_asap7.regfile \
      -check_ports generate_regfile_asap7_rtl_extra.v
  }
} message]
check "a port the spec does not name is refused" { set failed } 1
check "the refusal is RAM-0050" { string match "*RAM-0050*" $message } 1
check "the refusal names the port" \
  { string match "*module port w2_en is not in the spec*" $log } 1

set lef_file [make_result_file generate_regfile_asap7.lef]
set lib_file [make_result_file generate_regfile_asap7.lib]
generate_regfile -spec generate_regfile_asap7.regfile \
  -check_ports generate_regfile_asap7_rtl.v \
  -lef $lef_file -liberty $lib_file

set words 8
set bits 4
set abits 3
for { set w 1 } { $w < $words } { incr w } {
  rf_store $w $bits [rf_word_value $w $bits]
}

foreach port { r0 r1 } {
  for { set a 0 } { $a < $words } { incr a } {
    rf_drive ${port}_addr $abits $a
    set expected [expr { $a == 0 ? 0 : [rf_word_value $a $bits] }]
    check "$port reads word $a" { rf_read ${port}_data $bits } $expected
  }
}

# With a hold term per bit (write_style mux, the default) a word's flops
# load the write data when the word is written and their own value
# otherwise.
proc check_writes { port other } {
  rf_drive_bit ${other}_en 0
  rf_drive_bit ${port}_en 1
  foreach a { 1 3 6 } {
    set data [expr { (~[rf_word_value $a 4]) & 15 }]
    rf_drive ${port}_addr 3 $a
    rf_drive ${port}_data 4 $data
    for { set w 1 } { $w < 8 } { incr w } {
      set expected [expr { $w == $a ? $data : [rf_word_value $w 4] }]
      check "$port writing word $a: word $w loads" { rf_next $w 4 } $expected
    }
  }
  rf_drive_bit ${port}_en 0
  for { set w 1 } { $w < 8 } { incr w } {
    check "no write: word $w holds" { rf_next $w 4 } [rf_word_value $w 4]
  }
}
check_writes w0 w1
check_writes w1 w0

check "the abstract names every port" { rf_ports_missing_from $lef_file lef } {}
check "the timing model names every port" \
  { rf_ports_missing_from $lib_file lib } {}

exit_summary
