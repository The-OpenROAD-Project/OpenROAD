# Coverage for rsz GlobalSizingPolicy.
# Runs the GLOBAL_SIZING phase and checks the resized netlist against a golden.
#
# This file runs single-threaded (serial Phase-B, inline). The companion
# global_sizing_threads.tcl runs the identical flow multi-threaded and diffs the
# SAME golden, so the pair asserts the parallel Jacobi sweep is deterministic
# and matches the serial result.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_global_sizing_config -presize_mode max_size_min_vt
repair_timing -setup -phases GLOBAL_SIZING
report_worst_slack -max -digits 3
