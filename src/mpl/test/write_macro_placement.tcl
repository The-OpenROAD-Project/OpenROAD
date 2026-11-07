# Test if the macro placement file is correctly generated.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/io_constraints1.def"

set_thread_count 0

# The result of the test is a .tcl file with the same name as the
# test file. To prevent the latter from being overwritten, we
# write the output to the results folder.
set tcl_file [make_result_file write_macro_placement.tcl]

rtl_macro_placer -report_directory [make_result_dir] \
  -write_macro_placement $tcl_file

diff_files write_macro_placement.tclok $tcl_file
