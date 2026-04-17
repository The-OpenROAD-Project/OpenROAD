# Double-height cell is placed at y=5600 (an FS row) -- a wrong-parity landing
# that puts its VDD pin on a VSS stripe and vice versa.  Detailed placement
# must move it to a valid y (N row), leaving the single-row anchor untouched.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef multi_height_power_align.lef
read_def multi_height_power_align.def
detailed_placement
check_placement

set def_file [make_result_file multi_height_power_align.def]
write_def $def_file
diff_files multi_height_power_align.defok $def_file
