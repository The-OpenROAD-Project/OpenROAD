# repair high fanout tie hi/low net
source "helpers.tcl"
source "resizer_helpers.tcl"
source "tie_fanout.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set def_filename [file join $result_dir "tie_fanout1.def"]
write_tie_hi_fanout_def $def_filename LOGIC1_X1/Z BUF_X1/A 5
read_def $def_filename

repair_tie_fanout LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties LOGIC1_X1
check_in_core
