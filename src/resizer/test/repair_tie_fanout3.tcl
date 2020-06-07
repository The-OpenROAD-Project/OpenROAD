# repair tie fanout non-liberty pin on net
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_lef repair_tie_fanout3.lef
read_verilog repair_tie_fanout3.v
link_design top

repair_tie_fanout LOGIC1_X1/Z

source "tie_fanout.tcl"
check_ties LOGIC1_X1

set repaired_filename [file join $result_dir "repair_tie_fanout3.def"]
write_def $repaired_filename
#diff_file repair_tie_fanout3.defok $repaired_filename
