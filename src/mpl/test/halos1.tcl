# Test if the packing engine can handle one fixed
# macro and two movable macros without generating
# overlap.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0
mpl::mpl_debug -fine -skip_steps
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file fixed_macros1.def]
write_def $def_file

diff_files fixed_macros1.defok $def_file
