# repair_tie_fanout -separation
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_tie_fanout2.def
repair_tie_fanout -separation 2 LOGIC1_X1/Z

source tie_fanout.tcl
check_ties LOGIC1_X1

set repaired_filename [file join $result_dir "repair_tie_fanout2.def"]
write_def $repaired_filename
diff_file repair_tie_fanout2.defok $repaired_filename
