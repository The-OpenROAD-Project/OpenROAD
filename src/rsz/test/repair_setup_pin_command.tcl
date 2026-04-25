# Probe testing-only rsz::repair_setup_pin behavior.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizedown.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_worst_slack -max
rsz::repair_setup_pin [get_pin r2/D]
report_worst_slack -max
