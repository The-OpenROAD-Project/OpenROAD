# Coverage for rsz GlobalSizingPolicy.
# Runs the GLOBAL_SIZING phase and checks the resized netlist against a golden.
#
# This file runs single-threaded (serial Phase-B, inline). The companion
# global_sizing_threads.tcl runs the identical flow multi-threaded and diffs the
# SAME golden, so the pair asserts the parallel Jacobi sweep is deterministic
# and matches the serial result.
source "helpers.tcl"

# Thread count and result-file stem are overridable by the _threads variant.
if { ![info exists global_sizing_threads] } {
  set global_sizing_threads 1
}
if { ![info exists global_sizing_result] } {
  set global_sizing_result "global_sizing"
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
read_sdc gcd_nangate45.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_thread_count $global_sizing_threads
repair_timing -setup -phases GLOBAL_SIZING

set verilog_file [make_result_file "${global_sizing_result}.v"]
write_verilog $verilog_file
check "global sizing netlist matches golden" \
  {diff_files global_sizing.vok $verilog_file} 0

exit_summary
