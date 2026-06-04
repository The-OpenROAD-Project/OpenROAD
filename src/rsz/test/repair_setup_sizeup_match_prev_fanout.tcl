# SizeUpMatchMove must reject a candidate when the previous driver has
# multiple fanouts, even if that previous driver is stronger.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizeup_match_prev_fanout.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_checks -fields input -digits 3

set_dont_use [get_lib_cells CLKBUF*]
repair_timing -setup -sequence "sizeup_match"
report_checks -fields input -digits 3
