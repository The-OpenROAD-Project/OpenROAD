# Test if the .DEF placement blockage is correctly handled
# as a hard constraint i.e., fixed macro.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/placement_blockages1.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 4.0

set def_file [make_result_file placement_blockages1.def]
write_def $def_file

diff_files placement_blockages1.defok $def_file
