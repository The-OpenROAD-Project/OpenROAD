# Probe WNS max_passes/iteration guard differences.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement
repair_design
repair_timing -setup -phases "WNS" -max_passes 1 -verbose
report_worst_slack -max
report_tns -digits 3
