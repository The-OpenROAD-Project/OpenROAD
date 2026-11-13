# Check placement beside a macro that extends beyond the core.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def obstruction2.def
detailed_placement
check_placement
