# Test if firm COVERs are properly ignored.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./Nangate45_io/dummy_pads.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

place_inst -cell DUMMY_BUMP -name "test_pad1" -orient R0 -status FIRM -loc "100 100"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 0.3

set def_file [make_result_file fixed_covers.def]
write_def $def_file

diff_files fixed_covers.defok $def_file
