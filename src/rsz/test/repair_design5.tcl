# repair_design set_dont_touch instance
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "repair_design5.def"]
write_hi_fanout_def $def_filename 35

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 10 clk1
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

set_dont_touch drvr
repair_design
report_instance drvr

unset_dont_touch drvr
repair_design
report_instance drvr
