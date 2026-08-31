# Test if halos are correctly generated using 2 when using miniumum channel arguments.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halo3.def"

set_thread_count 0
rtl_macro_placer -min_channel_size {24 12} -report_directory [make_result_dir]

set def_file [make_result_file halos3.def]
write_def $def_file

diff_files halos3.defok $def_file
