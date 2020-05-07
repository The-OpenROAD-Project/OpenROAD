# repair_max_fanout -> resize
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [file join $result_dir "hi_fanout.def"]
write_hi_fanout_def $def_filename 35

read_liberty liberty1.lib
read_lef liberty1.lef
read_def $def_filename
create_clock -period 10 clk1

report_check_types -max_slew

repair_max_fanout -max_fanout 30 -buffer_cell liberty1/snl_bufx1
report_check_types -max_slew

resize
report_check_types -max_slew
