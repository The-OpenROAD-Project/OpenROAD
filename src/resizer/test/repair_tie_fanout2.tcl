# repair_tie_fanout -separation
source "helpers.tcl"
source "tie_fanout.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
# This is not generated because it has mirrored instances
read_def repair_tie_fanout2.def

repair_tie_fanout -separation 2 LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties LOGIC1_X1

set repaired_filename [file join $result_dir "repair_tie_fanout2.def"]
write_def $repaired_filename
#diff_file repair_tie_fanout2.defok $repaired_filename
