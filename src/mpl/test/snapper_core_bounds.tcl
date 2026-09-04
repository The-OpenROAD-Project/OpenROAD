# Check that snapped macro placement stays within the core.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/orientation_improve1.lef
read_def testcases/orientation_improve1.def

set_io_pin_constraint -direction INPUT -region right:10-30*
set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]

set block [ord::get_db_block]
set core [$block getCoreArea]
foreach inst [$block getInsts] {
  set master [$inst getMaster]
  if { [$master isBlock] } {
    set bbox [$inst getBBox]
    if { [$bbox xMin] < [$core xMin]
         || [$bbox yMin] < [$core yMin]
         || [$bbox xMax] > [$core xMax]
         || [$bbox yMax] > [$core yMax] } {
      error "Macro [$inst getName] is outside the core"
    }
  }
}
