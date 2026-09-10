# Centered (non-origin) floorplan: DIEAREA lower-left is not (0,0).
# Regression guard for #6704 -- detailed_placement must legalize a centered
# floorplan the same way it does an origin-anchored one (no exploded
# displacement, clean check_placement).
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def centered_floorplan.def
detailed_placement
check_placement

set def_file [make_result_file centered_floorplan.def]
write_def $def_file
diff_file centered_floorplan.defok $def_file
