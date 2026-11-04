# LEGACY repair_setup on a critical-path slice of Nangate45 ibex_core with 21
# violating endpoints. The golden log covers buffer removal, sizing up, pin
# swaps, buffering and cloning, and its worst path runs from a flip-flop to a
# reset recovery check.

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_path13.def
read_sdc repair_setup_path13.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
