# Exercise rtl_macro_placer twice in one OpenROAD process.
# The first run locks the macros. The second run must not dereference the
# hierarchy tree cleared by the first run.

source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/orientation_improve1.lef
read_def testcases/halos1.def

set_thread_count 0
set first_report [make_result_dir]
set second_report [make_result_dir]

rtl_macro_placer -report_directory $first_report
rtl_macro_placer -report_directory $second_report
