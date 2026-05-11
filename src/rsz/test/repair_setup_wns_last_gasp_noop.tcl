source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizedown.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
repair_timing -setup -phases "WNS LEGACY LAST_GASP" -max_passes 20 -verbose
report_worst_slack -max
report_tns -digits 3
