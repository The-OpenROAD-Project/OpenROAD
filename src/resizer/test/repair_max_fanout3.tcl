# repair_max_fanout -> resize
source "helpers.tcl"
source "hi_fanout.tcl"
source "report_max_fanout.tcl"

set def_filename [file join $result_dir "repair_max_fanout3.def"]
write_hi_fanout_def $def_filename 35

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 10 clk1

report_max_fanout 1
repair_max_fanout -max_fanout 30 -buffer_cell BUF_X1
report_max_fanout 3

resize
