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
set default_seed_origin [$macro getOrigin]

$macro setPlacementStatus PLACED
rtl_macro_placer -report_directory [make_result_dir]
check "placed after a completed run" { $macro getPlacementStatus } LOCKED

proc reset_macro_for_rerun { macro } {
  foreach blockage [[ord::get_db_block] getBlockages] {
    odb::dbBlockage_destroy $blockage
  }
  $macro setPlacementStatus PLACED
}

reset_macro_for_rerun $macro
rtl_macro_placer -random_seed 7 -report_directory [make_result_dir]
check "placed with explicit -random_seed" { $macro getPlacementStatus } LOCKED

reset_macro_for_rerun $macro
rtl_macro_placer -report_directory [make_result_dir]
check "-random_seed resets to default when omitted on subsequent call" \
  { $macro getOrigin } $default_seed_origin

reset_macro_for_rerun $macro
rtl_macro_placer -anneal_end_acceptance 0.0001 -anneal_patience 0.2 \
  -report_directory [make_result_dir]
check "placed with explicit -anneal_end_acceptance and -anneal_patience" \
  { $macro getPlacementStatus } LOCKED

reset_macro_for_rerun $macro
rtl_macro_placer -report_directory [make_result_dir]
check "-anneal_end_acceptance and -anneal_patience reset when omitted on subsequent call" \
  { $macro getOrigin } $default_seed_origin

check_error "MPL-0081 thrown when -anneal_end_acceptance is out of (0.0, 0.9)" {
  rtl_macro_placer -anneal_end_acceptance 0.95 -report_directory [make_result_dir]
} "MPL-0081"

check_error "MPL-0082 thrown when -anneal_patience is out of \[0.0, 1.0\]" {
  rtl_macro_placer -anneal_patience 1.2 -report_directory [make_result_dir]
} "MPL-0082"

check_error "MPL-0083 thrown when -placement_seeds is less than 1" {
  rtl_macro_placer -placement_seeds 0 -report_directory [make_result_dir]
} "MPL-0083"

reset_macro_for_rerun $macro
rtl_macro_placer -placement_seeds 3 -anneal_end_acceptance 0.0001 \
  -anneal_patience 0.2 -random_seed 5 -report_directory [make_result_dir]
check "placed with explicit -placement_seeds 3" \
  { $macro getPlacementStatus } LOCKED

reset_macro_for_rerun $macro
rtl_macro_placer -report_directory [make_result_dir]
check "-placement_seeds resets to default when omitted on subsequent call" \
  { $macro getOrigin } $default_seed_origin

# Verify thread-count determinism (identical DEF output across 1, 8, and 16 threads)
set def_files {}
foreach threads {1 8 16} {
  set_thread_count $threads
  reset_macro_for_rerun $macro
  rtl_macro_placer -placement_seeds 3 -anneal_end_acceptance 0.0001 \
    -anneal_patience 0.2 -random_seed 11 -report_directory [make_result_dir]
  set def_file [make_result_file "rerun1_threads_${threads}.def"]
  write_def $def_file
  lappend def_files $def_file
}
check "identical DEF output between 1 and 8 threads" \
  { diff_files [lindex $def_files 0] [lindex $def_files 1] } 0
check "identical DEF output between 1 and 16 threads" \
  { diff_files [lindex $def_files 0] [lindex $def_files 2] } 0
set_thread_count 0

# Verify MPL-0003 reports child macro minimum area as a percentage of outline
# area when coarse shaping finds no valid tilings.
set macro1 [[ord::get_db_block] findInst MACRO_1]
reset_macro_for_rerun $macro1
reset_macro_for_rerun $macro
check_error "MPL-0003 thrown when no valid tilings exist" {
  rtl_macro_placer -min_channel_size 30 -report_directory [make_result_dir]
} "MPL-0003"

exit_summary
