# repair_design liberty max_cap/max_slew/max_fanout 0.0 (nonsense values)
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty repair_design1.lib
read_lef Nangate45/Nangate45.lef
set def_file [make_result_file "repair_design1.def"]
write_hi_fanout_def $def_file 10
read_def $def_file
create_clock -period 1 clk1

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_design

report_check_types -max_slew -max_cap -max_fanout
