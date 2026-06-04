# Test if base halos are correctly generated using 4 arguments.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halo3.def"

set_thread_count 0
set_macro_base_halo 16.0 8.0 4.0 2.0
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file halos4.def]
write_def $def_file

diff_files halos4.defok $def_file
