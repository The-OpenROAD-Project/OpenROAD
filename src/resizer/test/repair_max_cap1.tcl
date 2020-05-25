# repair_max_cap hi fanout register array
source "helpers.tcl"
source "hi_fanout.tcl"
source "report_max_cap.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set def_file [file join $result_dir "repair_max_cap1.def"]
write_hi_fanout_def $def_file 40
read_def $def_file
create_clock -period 1 clk1

report_max_cap 2

repair_max_cap -buffer_cell BUF_X2
report_max_cap 4
