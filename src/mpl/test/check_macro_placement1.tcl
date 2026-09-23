# check_macro_placement runs the checks rtl_macro_placer makes before it
# clusters, and stops: the design is unchanged and no report directory is
# written. Each failing check is the error rtl_macro_placer gives for it,
# the check can run again after an error, and rtl_macro_placer runs
# normally after it.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

set block [ord::get_db_block]
set macro_1 [$block findInst MACRO_1]
set macro_2 [$block findInst MACRO_2]

proc snapshot { block } {
  set result {}
  foreach inst [$block getInsts] {
    lappend result [list [$inst getName] {*}[$inst getLocation] \
      [$inst getOrient] [$inst getPlacementStatus]]
  }
  return $result
}

proc check_quietly { args } {
  upvar 1 log log message message
  # tee evaluates its command in its own scope, so build the command first.
  set cmd [list check_macro_placement {*}$args]
  return [catch { tee -quiet -variable log $cmd } message]
}

set_thread_count 0
set design [snapshot $block]

# Every macro fixed: nothing to place.
set macro_2_status [$macro_2 getPlacementStatus]
$macro_2 setPlacementStatus FIRM
check "every macro fixed: the check returns" { check_quietly } 0
check "every macro fixed: nothing to place" { set message } 0
check "every macro fixed: MPL-0017" { string match {*MPL-0017*} $log } 1
$macro_2 setPlacementStatus $macro_2_status

# A feasible design: the checks pass and nothing changes.
check "feasible: the check returns" { check_quietly } 0
check "feasible: a macro to place" { set message } 1
check "feasible: MPL-0079" { string match {*MPL-0079*} $log } 1
check "feasible: the design is unchanged" { expr { [snapshot $block] eq $design } } 1
check "feasible: no report directory" { file exists hier_rtlmp } 0

# A fixed standard cell inside the macro placement area.
place_inst -cell BUF_X1 -name fixed_buf -orient R0 -status FIRM -loc "20 20"
set design_with_buf [snapshot $block]
check "fixed cell: the check fails" { check_quietly } 1
check "fixed cell: MPL-0050" { string match {*MPL-0050*} $message } 1
check "fixed cell: the log names the cell" { string match {*MPL-0050*fixed_buf*} $log } 1
check "fixed cell: the design is unchanged" \
  { expr { [snapshot $block] eq $design_with_buf } } 1
set failed [catch { rtl_macro_placer -report_directory [make_result_dir] } placer_message]
check "fixed cell: rtl_macro_placer fails the same way" \
  { expr { $failed && [string match {*MPL-0050*} $placer_message] } } 1
odb::dbInst_destroy [$block findInst fixed_buf]

# Macro halos: a macro that no longer fits in the core with its halo, and
# -use_full_halo reaching the checks as it reaches rtl_macro_placer.
set_macro_base_halo 70 70
check "halo too wide: the check fails" { check_quietly } 1
check "halo too wide: MPL-0006" { string match {*MPL-0006*} $message } 1
set failed [catch { rtl_macro_placer -report_directory [make_result_dir] } placer_message]
check "halo too wide: rtl_macro_placer fails the same way" \
  { expr { $failed && [string match {*MPL-0006*} $placer_message] } } 1
set_macro_base_halo 58 58
check "pin-aware halo: the check passes" { check_quietly } 0
check "full halo: the check fails" { check_quietly -use_full_halo } 1
check "full halo: MPL-0065" { string match {*MPL-0065*} $message } 1
set failed [catch {
  rtl_macro_placer -use_full_halo -report_directory [make_result_dir]
} placer_message]
check "full halo: rtl_macro_placer fails the same way" \
  { expr { $failed && [string match {*MPL-0065*} $placer_message] } } 1
set_macro_base_halo 0 0

# The global fence reaches the checks as it reaches rtl_macro_placer: one
# too small for the movable cells fails, one large enough passes.
check "small fence: the check fails" \
  { check_quietly -fence_lx 0 -fence_ly 0 -fence_ux 90 -fence_uy 90 } 1
check "small fence: MPL-0065" { string match {*MPL-0065*} $message } 1
set failed [catch {
  rtl_macro_placer -fence_lx 0 -fence_ly 0 -fence_ux 90 -fence_uy 90 \
    -report_directory [make_result_dir]
} placer_message]
check "small fence: rtl_macro_placer fails the same way" \
  { expr { $failed && [string match {*MPL-0065*} $placer_message] } } 1
check "large fence: the check passes" \
  { check_quietly -fence_lx 0 -fence_ly 0 -fence_ux 150 -fence_uy 150 } 0

# After the errors the check passes again and has left the design alone.
check "again: the check passes" { check_quietly } 0
check "again: the design is unchanged" { expr { [snapshot $block] eq $design } } 1

# rtl_macro_placer is unaffected by the checks before it.
set failed [catch { rtl_macro_placer -report_directory [make_result_dir] } placer_message]
if { $failed } {
  puts $placer_message
}
check "rtl_macro_placer runs after the checks" { set failed } 0
set status [$macro_2 getPlacementStatus]
check "rtl_macro_placer placed the movable macro" \
  { expr { $status ne "NONE" && $status ne "UNPLACED" } } 1

exit_summary
