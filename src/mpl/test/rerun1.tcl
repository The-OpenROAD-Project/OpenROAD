# rtl_macro_placer called more than once in one session places what is
# unfixed each time: after a call that found every macro fixed, and after
# a completed run.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

set_thread_count 0
set macro [[ord::get_db_block] findInst MACRO_2]

$macro setPlacementStatus FIRM
rtl_macro_placer -report_directory [make_result_dir]

$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]
check "placed after a call with nothing to place" \
  { $macro getPlacementStatus } LOCKED

$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]
check "placed after a completed run" { $macro getPlacementStatus } LOCKED

exit_summary
