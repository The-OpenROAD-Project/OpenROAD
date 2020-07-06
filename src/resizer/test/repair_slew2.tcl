# repair_design max_slew liberty pin max_transition
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty repair_slew2.lib
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set def_file [file join $result_dir "repair_slew2.def"]
write_hi_fanout_def $def_file 20
read_def $def_file
create_clock -period 1 clk1

report_check_types -max_slew -violators
repair_design -buffer_cell BUF_X2
report_check_types -max_slew -violators
