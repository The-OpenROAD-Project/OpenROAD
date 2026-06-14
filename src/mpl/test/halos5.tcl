# Test that -use_full_halo restores uniform halo behavior,
# ignoring pin-aware detection (regression for the use_full_halo flag).
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -use_full_halo

set def_file [make_result_file halos5.def]
write_def $def_file

diff_files halos5.defok $def_file
