# Probe SizeDownMove vs SizeDownGenerator/Candidate behavior.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizedown.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
set_dont_use [get_lib_cells CLKBUF*]
report_worst_slack -max
repair_timing -setup -sequence "sizedown" -skip_last_gasp -max_passes 3 -verbose
report_worst_slack -max
report_tns -digits 3
