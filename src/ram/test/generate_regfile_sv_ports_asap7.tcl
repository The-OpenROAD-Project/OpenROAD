# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Test: ram/generate_regfile_sv_ports_asap7
# The port check reads a SystemVerilog module header: a #(...) parameter
# list, port widths written in the parameters' defaults, and an input the
# register file does not use (`unused`). It refuses a width that differs
# once the parameters are evaluated, and a declaration it cannot read,
# rather than skipping it.

source "helpers.tcl"
source "regfile_checks.tcl"

rf_read_libraries
set spec [rf_spec generate_regfile_asap7.regfile \
  generate_regfile_sv_ports_asap7.regfile {"unused test_en_i"}]

# The same module with other text: a copy of the RTL in the results dir.
proc rtl_variant { name from to } {
  set in [open generate_regfile_sv_ports_asap7_rtl.sv r]
  set text [read $in]
  close $in
  set file [make_result_file $name]
  set out [open $file w]
  puts -nonewline $out [string map [list $from $to] $text]
  close $out
  return $file
}

# generate_regfile's error and its log for `rtl`; tee runs its script
# unsubstituted, so the command is built first.
proc refusal { rtl } {
  global spec
  set cmd [list generate_regfile -spec $spec -check_ports $rtl]
  set failed [catch { tee -quiet -variable log $cmd } message]
  return [list $failed $message $log]
}

lassign [refusal [rtl_variant wide.sv "W = 4" "W = 8"]] failed message log
check "a width that differs once W is evaluated is refused" { set failed } 1
check "the refusal is RAM-0050" { string match "*RAM-0050*" $message } 1
check "the refusal names the port and both widths" \
  { string match "*port r0_data is 8 bits in the module, 4 in the spec*" $log } 1

lassign [refusal [rtl_variant typed.sv "logic \[A-1:0\] r1_addr" \
  "rf_pkg::addr_t r1_addr"]] failed message log
check "a declaration the check cannot read is refused" { set failed } 1
check "the refusal quotes it" \
  { string match "*cannot read the port declaration*rf_pkg::addr_t r1_addr*" $log } 1

generate_regfile -spec $spec -check_ports generate_regfile_sv_ports_asap7_rtl.sv
check "the unused input is a port of the register file" \
  { expr { [[ord::get_db_block] findBTerm test_en_i] ne "NULL" } } 1

for { set w 1 } { $w < 8 } { incr w } {
  rf_store $w 4 [rf_word_value $w 4]
}
for { set a 0 } { $a < 8 } { incr a } {
  rf_drive r1_addr 3 $a
  set expected [expr { $a == 0 ? 0 : [rf_word_value $a 4] }]
  check "r1 reads word $a" { rf_read r1_data 4 } $expected
}

exit_summary
