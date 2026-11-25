# fully_rebuffer with set_opt_config -rebuffer_size_driver on sibling drivers
# that share a weakly driven input net.  Three NAND2_X1 gates on one STA level
# take A1 from port "in" (20 kohm drive) and their failing path from the late
# port "a" on A2.  Upsizing one sibling raises its A1 capacitance and slows
# "in", so the next sibling must be re-timed before it is sized.  On stale
# timing every sibling upsizes and the A1 paths through "in" become critical.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Net connected to a placed top-level port.
proc make_port { name io x y } {
  global block pin_layer
  set net [odb::dbNet_create $block $name]
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
  return $net
}

# Placed instance of master with {term net} connections.
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns {
    [$inst findITerm $term] connect $net
  }
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  return $inst
}

# Siblings g0..g2 share nets "in" and "a"; each drives a dont_touch INV_X16
# load (25 fF) next to its output port.
set in [make_port in INPUT 0 0]
set a [make_port a INPUT 0 [ord::microns_to_dbu 1]]
for { set i 0 } { $i < 3 } { incr i } {
  set y [ord::microns_to_dbu [expr { 3 * $i }]]
  set z [odb::dbNet_create $block z$i]
  set out [make_port out$i OUTPUT [ord::microns_to_dbu 24] $y]
  make_inst NAND2_X1 g$i [list A1 $in A2 $a ZN $z] [ord::microns_to_dbu 4] $y
  set load [make_inst INV_X16 l$i [list A $z ZN $out] [ord::microns_to_dbu 20] $y]
  $load setDoNotTouch true
}
ord::design_created

# The A2 paths from "a" fail; the A1 paths through "in" have slack to lose.
create_clock -period 0.4 -name clk
set_input_delay -clock clk 0 [get_ports in]
set_input_delay -clock clk 0.35 [get_ports a]
set_output_delay -clock clk 0 [all_outputs]
set_drive 20 [get_ports in]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Master of each sibling.
proc report_siblings { } {
  set masters {}
  foreach name { g0 g1 g2 } {
    lappend masters "$name [get_property [get_cells $name] ref_name]"
  }
  puts [join $masters ", "]
}

report_siblings
report_worst_slack -max
report_tns

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_siblings
report_worst_slack -max
report_tns

reset_opt_config -rebuffer_size_driver
