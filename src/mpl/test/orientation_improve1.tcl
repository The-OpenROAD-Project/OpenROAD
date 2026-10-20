# Test orientation improvement with one constrained pin.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/orientation_improve1.def"

set_io_pin_constraint -direction INPUT -region right:10-30*

set_thread_count 0
set tcl_file [make_result_file orientation_improve1.tcl]
rtl_macro_placer -report_directory [make_result_dir] \
  -write_macro_placement $tcl_file

source $tcl_file

set def_file [make_result_file orientation_improve1.def]
write_def $def_file

diff_files orientation_improve1.defok $def_file
