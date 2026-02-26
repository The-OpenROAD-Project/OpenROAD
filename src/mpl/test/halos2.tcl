# Test if the packing engine can handle one fixed
# macro and two movable macros without generating
# overlap.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/halos1.def"

set_thread_count 0
set_macro_halo -macro MACRO_1 -halo {8 4 2 1}
set_macro_halo -macro MACRO_2 -halo {2 4}
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file halos2.def]
write_def $def_file

diff_files halos2.defok $def_file
