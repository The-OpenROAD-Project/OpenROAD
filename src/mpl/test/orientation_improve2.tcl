# Test orientation improvement with one unconstrained pin.
# All edges are blocked, except one.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/orientation_improve1.def"

exclude_io_pin_region -region bottom:* -region right:* -region top:*

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 0.3

set def_file [make_result_file orientation_improve2.def]
write_def $def_file

diff_files orientation_improve2.defok $def_file
