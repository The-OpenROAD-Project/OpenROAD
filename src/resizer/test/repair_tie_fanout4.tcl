# repair high fanout tie hi/low net
source helpers.tcl
source tie_fanout.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set def_filename [file join $result_dir "tie_hi2.def"]
write_tie_hi_fanout_def $def_filename LOGIC1_X1/Z BUF_X1/A 5

read_def $def_filename

# repair_max_fanout should NOT repair the tie hi/low nets.
set_max_fanout 100 [current_design]
repair_max_fanout -buffer_cell BUF_X1
repair_tie_fanout LOGIC1_X1/Z

source "tie_fanout.tcl"
check_ties LOGIC1_X1

set repaired_filename [file join $result_dir "repair_tie_fanout4.def"]
write_def $repaired_filename
diff_file repair_tie_fanout4.defok $repaired_filename
