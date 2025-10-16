# test for pdngen -ripup
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef

read_def nangate_gcd/floorplan_with_grid.def

pdngen -ripup

set def_file [make_result_file ripup.def]
write_def $def_file
diff_files ripup.defok $def_file
