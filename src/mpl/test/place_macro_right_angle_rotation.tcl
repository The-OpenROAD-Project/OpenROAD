# Test if a macro on R90 orientation is correctly placed by place_macro.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/place_macro_right_angle_rotation.lef"

read_def "./testcases/place_macro_right_angle_rotation.def"

# Exercise overlap checks when the target macro is already placed.
set block [ord::get_db_block]
set macro [$block findInst macro]
$macro setLocation 40000 40000
$macro setPlacementStatus PLACED

place_macro -macro_name macro -location {20 20} -orientation R90

set def_file [make_result_file place_macro_right_angle_rotation.def]
write_def $def_file

diff_files place_macro_right_angle_rotation.defok $def_file
