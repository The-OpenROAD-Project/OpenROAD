# Probe move_tracker level-1 reporting gates in custom WNS phase.
source "helpers.tcl"
set_debug_level RSZ move_tracker 1
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement
repair_design
repair_timing -setup -phases "WNS" -max_passes 2
