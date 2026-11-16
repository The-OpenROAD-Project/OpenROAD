# LEGACY repair_setup on a critical-path slice of Nangate45 gcd with 5
# violating endpoints. The golden log covers sizing up, pin swaps and
# buffering, and its worst path runs from a flip-flop to an output port.

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_path12.def
read_sdc repair_setup_path12.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4
