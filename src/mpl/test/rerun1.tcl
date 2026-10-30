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
set default_seed_origin [$macro getOrigin]

$macro setPlacementStatus PLACED
rtl_macro_placer -random_seed 7 -report_directory [make_result_dir]
check "placed with explicit -random_seed" { $macro getPlacementStatus } LOCKED

$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]
check "-random_seed resets to default when omitted on subsequent call" \
  { $macro getOrigin } $default_seed_origin

# Verify MPL-0003 reports child macro minimum area as a percentage of outline
# area when coarse shaping finds no valid tilings.
set macro1 [[ord::get_db_block] findInst MACRO_1]
$macro1 setLocation 200000 200000
$macro1 setPlacementStatus FIRM
$macro setPlacementStatus PLACED
set mpl0003_caught [catch {
  rtl_macro_placer -min_channel_size 30 -report_directory [make_result_dir]
} mpl0003_msg]
check "MPL-0003 thrown when no valid tilings exist" \
  { expr { $mpl0003_caught == 1 && [string first "MPL-0003" $mpl0003_msg] != -1 } } 1
check "MPL-0003 message includes macro area percentage of outline" \
  { regexp {[0-9]+\.[0-9]+%} $mpl0003_msg } 1

exit_summary
