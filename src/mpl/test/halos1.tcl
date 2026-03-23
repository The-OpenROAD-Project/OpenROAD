# Test if MPL handles DEF and directional halos
# correctly
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file halos1.def]
write_def $def_file

diff_files halos1.defok $def_file
