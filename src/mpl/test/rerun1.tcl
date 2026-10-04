# rtl_macro_placer called more than once in one session. A call that finds
# every macro fixed must not make the next call skip placement, and a call
# after a completed run must start from a fresh hierarchy.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

set_thread_count 0
set block [ord::get_db_block]
set macro [$block findInst MACRO_2]

# Nothing to place.
$macro setPlacementStatus FIRM
rtl_macro_placer -report_directory [make_result_dir]

# One macro to place.
$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]

# The same again, after a completed run.
$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file rerun1.def]
write_def $def_file

diff_files rerun1.defok $def_file
