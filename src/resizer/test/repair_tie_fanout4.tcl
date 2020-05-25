# repair high fanout tie hi/low net
source helpers.tcl
source tie_fanout.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set def_filename [file join $result_dir "tie_hi2.def"]
write_tie_hi_fanout_def $def_filename LOGIC1_X1/Z BUF_X1/A 35

read_def $def_filename

# These should NOT repair the tie hi/low nets.
repair_max_fanout -max_fanout 10 -buffer_cell BUF_X1
repair_tie_fanout -max_fanout 10 LOGIC1_X1/Z
