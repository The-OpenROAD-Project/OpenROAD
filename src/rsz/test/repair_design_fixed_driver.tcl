# repair_design sizes drivers to repair slew and capacitance, but a FIRM
# instance keeps its master: a wider master cannot move away from the
# fixed cells around it, and the legalizer cannot move it either. Two
# identical nets with a max cap and slew violation: the PLACED driver is
# upsized, the FIRM driver, abutted by a FIRM neighbour, is not.
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "repair_design_fixed_driver.def"]
write_hi_fanout_def1 $def_filename 5 \
  "drvr" "BUF_X1" "" "Z" \
  "load" "DFF_X1" "CK" "D" 5000 \
  "metal1" 1000

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename

# A second, identical net whose driver is FIRM with a FIRM cell abutting it.
set db [ord::get_db]
set block [ord::get_db_block]
set buf [$db findMaster BUF_X1]
set dff [$db findMaster DFF_X1]
set fixed_drvr [odb::dbInst_create $block $buf fixed_drvr]
set neighbour [odb::dbInst_create $block $buf neighbour]
set fixed_load [odb::dbInst_create $block $dff fixed_load]
set net1 [odb::dbNet_create $block net1]
[$fixed_drvr findITerm Z] connect $net1
[$fixed_load findITerm D] connect $net1
[$fixed_load findITerm CK] connect [$block findNet clk1]
set dbu [$block getDbUnitsPerMicron]
$fixed_drvr setLocation [expr { 10 * $dbu }] [expr { 10 * $dbu }]
$fixed_drvr setPlacementStatus FIRM
$neighbour setLocation [expr { 10 * $dbu + [$buf getWidth] }] [expr { 10 * $dbu }]
$neighbour setPlacementStatus FIRM
$fixed_load setLocation [expr { 20 * $dbu }] [expr { 10 * $dbu }]
$fixed_load setPlacementStatus PLACED

create_clock -period 10 clk1
source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement
set_load 200 net0
set_load 200 net1

proc master { name } {
  return [get_property [get_cells $name] ref_name]
}

proc overlaps { a b } {
  set block [ord::get_db_block]
  set box_a [[$block findInst $a] getBBox]
  set box_b [[$block findInst $b] getBBox]
  return [expr {
    [$box_a xMin] < [$box_b xMax] && [$box_b xMin] < [$box_a xMax]
    && [$box_a yMin] < [$box_b yMax] && [$box_b yMin] < [$box_a yMax]
  }]
}

set failed [catch { tee -quiet -variable log { repair_design } } message]
if { $failed } {
  puts $log
  puts $message
}
check "repair_design returns" { set failed } 0
check "the PLACED driver is upsized" { expr { [master drvr] ne "BUF_X1" } } 1
check "the FIRM driver keeps its master" { master fixed_drvr } BUF_X1
check "the FIRM driver is still FIRM" \
  { [$block findInst fixed_drvr] getPlacementStatus } FIRM
check "the FIRM driver does not overlap its neighbour" \
  { overlaps fixed_drvr neighbour } 0

exit_summary
