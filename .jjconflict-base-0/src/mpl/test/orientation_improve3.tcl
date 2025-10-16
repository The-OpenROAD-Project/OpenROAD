# Test orientation improvement with one fixed pin.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/orientation_improve1.def"

place_pin -pin_name io_1 -layer metal3 -force_to_die_boundary -location {150 20}

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 0.3

set def_file [make_result_file orientation_improve3.def]
write_def $def_file

diff_files orientation_improve3.defok $def_file
