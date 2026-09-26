# A fixed standard cell inside the macro placement area is refused only
# when there are macros to place. With every macro fixed the placer has
# nothing to do and says so (MPL-0017) instead of failing on the cell
# (MPL-0050).
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/fixed_macros1.def"

set block [ord::get_db_block]

proc placement { inst } {
  return [list {*}[$inst getLocation] [$inst getOrient] [$inst getPlacementStatus]]
}

proc macros { block } {
  set result {}
  foreach inst [$block getInsts] {
    if { [$inst isBlock] } {
      lappend result $inst
    }
  }
  return $result
}

foreach macro [macros $block] {
  $macro setPlacementStatus FIRM
}
place_inst -cell BUF_X1 -name fixed_buf -orient R0 -status FIRM -loc "20 20"
set fixed_buf [$block findInst fixed_buf]

set before {}
foreach inst [concat [macros $block] $fixed_buf] {
  lappend before [placement $inst]
}

set_thread_count 0
set failed [catch {
  tee -quiet -variable log {
    rtl_macro_placer -report_directory [make_result_dir]
  }
} message]
if { $failed } {
  puts $message
}
check "every macro fixed: the placer returns" { set failed } 0
check "every macro fixed: nothing to place is reported" \
  { string match {*MPL-0017*} $log } 1
check "every macro fixed: the fixed cell is not an error" \
  { string match {*MPL-0050*} $log } 0

set after {}
foreach inst [concat [macros $block] $fixed_buf] {
  lappend after [placement $inst]
}
check "every macro fixed: nothing moved" { expr { $after eq $before } } 1

# With a macro to place, the same fixed cell in the macro placement area is
# still an error, and names the cell.
[$block findInst MACRO_2] setPlacementStatus PLACED
set failed [catch {
  tee -quiet -variable log {
    rtl_macro_placer -report_directory [make_result_dir]
  }
} message]
check "a macro to place: the fixed cell is refused" { set failed } 1
check "a macro to place: the error is MPL-0050" \
  { string match {*MPL-0050*} $message } 1
check "a macro to place: the error names the cell" \
  { string match {*MPL-0050*fixed_buf*} $log } 1

exit_summary
