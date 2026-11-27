# fully_rebuffer with set_opt_config -rebuffer_size_driver counts a downsize
# as load given back to a shared input net.  NAND2 siblings g0..g2 on one STA
# level take A1 from the weakly driven port "in" and their failing paths from
# their own late ports a0..a2 on A2.  g1 starts as NAND2_X4 with a relaxed
# output and is downsized; g2 is upsized to NAND2_X4.  The net's pin load
# ends where it started, so g0 may still upsize; summing only the increases
# would hold "in" past the cap growth limit and keep g0 at NAND2_X1.
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

# Each g drives a dont_touch INV_X16 load (25 fF) next to its output port.
set in [make_port in INPUT 0 0]
for { set i 0 } { $i < 3 } { incr i } {
  set y [ord::microns_to_dbu [expr { 3 * $i }]]
  set a [make_port a$i INPUT 0 [ord::microns_to_dbu 1]]
  set z [odb::dbNet_create $block z$i]
  set out [make_port out$i OUTPUT [ord::microns_to_dbu 24] $y]
  set master [expr { $i == 1 ? "NAND2_X4" : "NAND2_X1" }]
  make_inst $master g$i [list A1 $in A2 $a ZN $z] [ord::microns_to_dbu 4] $y
  set load [make_inst INV_X16 l$i [list A $z ZN $out] [ord::microns_to_dbu 20] $y]
  $load setDoNotTouch true
}
ord::design_created

# The paths through g0 and g2 fail; out1 is relaxed, so g1 may downsize.
create_clock -period 0.4 -name clk
set_input_delay -clock clk 0.35 [get_ports in]
set_input_delay -clock clk 0.35 [get_ports a*]
set_output_delay -clock clk 0 [all_outputs]
set_output_delay -clock clk -1 [get_ports out1]
set_drive 0.35 [get_ports in]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Masters of g0..g2 and the pin capacitance of "in".
proc report_state { } {
  set masters {}
  foreach name { g0 g1 g2 } {
    lappend masters "$name [get_property [get_cells $name] ref_name]"
  }
  puts [join $masters ", "]
  report_net -digits 3 in
  report_worst_slack -max -digits 3
  report_tns -digits 3
}

report_state

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
