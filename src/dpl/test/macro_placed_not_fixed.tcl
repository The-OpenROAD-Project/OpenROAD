# A macro that is PLACED but not FIXED should emit DPL-404 and be treated as
# fixed (kept as an obstacle, not legalized as a movable cell).
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef block2.lef
read_def cell_on_block1.def

# cell_on_block1.def marks the macro FIXED; demote it to PLACED to exercise
# the auto-fix path in createNetwork.
set block [ord::get_db_block]
set macro [$block findInst block1]
$macro setPlacementStatus PLACED

detailed_placement

set def_file [make_result_file macro_placed_not_fixed.def]
write_def $def_file
diff_file macro_placed_not_fixed.defok $def_file
