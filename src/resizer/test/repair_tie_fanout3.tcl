# repair tie fanout non-liberty pin on net
source "helpers.tcl"
source "tie_fanout.tcl"
read_liberty Nangate45/Nangate45_typ.lib
# intentionally skip read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_def repair_tie_fanout3.def

repair_tie_fanout LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties LOGIC1_X1

set repaired_filename [file join $result_dir "repair_tie_fanout3.def"]
write_def $repaired_filename
#diff_file repair_tie_fanout3.defok $repaired_filename
