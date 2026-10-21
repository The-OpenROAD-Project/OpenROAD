# STARTPOINT_FANOUT phase with violating top-level input port startpoints.
# A port has no liberty driver and must not be collected as a repair pin.
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_swappins_legacy_flat.def

create_clock [get_ports clk] -period 0.1
set_clock_uncertainty 0 [get_clocks clk]
set_input_delay -clock clk 0.02 [all_inputs -no_clocks]
set_output_delay -clock clk 0.05 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement

repair_timing -setup -phases "STARTPOINT_FANOUT" -max_passes 2 -verbose
report_worst_slack -max
report_tns -digits 3
