# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_reset_high_asap7
# generate_regfile with an asynchronous reset, active high (async_reset
# rst high): every storage flop is the spec's reset flop, its reset
# (RESETN, which clears the stored value) is asserted exactly while rst
# is active, its set is tied off, and the file reads its words once reset
# is released. Checked by constant propagation through the generated
# cells.

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries
set spec [rf_spec generate_regfile_asap7.regfile generate_regfile_reset_high_asap7.regfile {
  "async_reset rst high"
  "cell flop_r DFFASRHQNx1_ASAP7_75t_R"
  "cell tie_hi TIEHIx1_ASAP7_75t_R"
}]
generate_regfile -spec $spec

set flops {}
for { set w 1 } { $w < 8 } { incr w } {
  for { set b 0 } { $b < 4 } { incr b } {
    lappend flops w${w}_b${b}_ff
  }
}
set flop_r [[ord::get_db] findMaster DFFASRHQNx1_ASAP7_75t_R]
foreach ff $flops {
  check "$ff is the reset flop" \
    { [[ord::get_db_block] findInst $ff] getMaster } $flop_r
}

proc check_reset { flops state value } {
  foreach ff $flops {
    check "rst $state: $ff RESETN" { rf_value $ff/RESETN } $value
    check "rst $state: $ff SETN is tied off" { rf_value $ff/SETN } 1
  }
}
rf_drive_bit rst 1
check_reset $flops active 0
rf_drive_bit rst 0
check_reset $flops released 1

for { set w 1 } { $w < 8 } { incr w } {
  rf_store $w 4 [rf_word_value $w 4]
}
for { set a 0 } { $a < 8 } { incr a } {
  rf_drive r0_addr 3 $a
  set expected [expr { $a == 0 ? 0 : [rf_word_value $a 4] }]
  check "released: r0 reads word $a" { rf_read r0_data 4 } $expected
}

exit_summary
