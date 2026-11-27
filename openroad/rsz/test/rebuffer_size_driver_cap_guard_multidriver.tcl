# fully_rebuffer with set_opt_config -rebuffer_size_driver measures the cap
# growth of a shared input net that has several drivers.  The topology is
# rebuffer_size_driver_shared_input, but two dont_touch INV_X1 in parallel
# drive the dont_touch net "in", and a false path through d1 leaves only d0
# timed.  Read from GraphDelayCalc::netCaps(), the pin load of such a net
# came from a cache that resizes did not update, so the guard saw no growth
# and all twelve siblings upsized; read live, it holds "in" after three.
# "in" is held as failing on d0's slack; judged by its first driver, d1,
# alone, it passed and all twelve siblings upsized.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Net connected to a placed top-level port at (x, y) um.
proc make_port { name io x y } {
  global block pin_layer
  set net [odb::dbNet_create $block $name]
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
  return $net
}

# Placed instance of master at (x, y) um with {term net} connections.
proc make_inst { master name conns x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  foreach { term net } $conns {
    [$inst findITerm $term] connect $net
  }
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
  return $inst
}

# Parallel drivers pi -> d0, d1 -> in; side path in -> s -> ls -> outs, all
# dont_touch.
set pi [make_port pi INPUT 0 0]
set in [odb::dbNet_create $block in]
for { set j 0 } { $j < 2 } { incr j } {
  set drv [make_inst INV_X1 d$j [list A $pi ZN $in] 1 $j]
  $drv setDoNotTouch true
}
set zs [odb::dbNet_create $block zs]
set outs [make_port outs OUTPUT 24 40]
set s [make_inst INV_X1 s [list A $in ZN $zs] 4 40]
$s setDoNotTouch true
set ls [make_inst BUF_X1 ls [list A $zs Z $outs] 20 40]
$ls setDoNotTouch true
# Siblings g0..g11, each with its own late port and a dont_touch INV_X16
# load (25 fF) next to its output port.
for { set i 0 } { $i < 12 } { incr i } {
  set y [expr { 3 * $i }]
  set a [make_port a$i INPUT 0 1]
  set z [odb::dbNet_create $block z$i]
  set out [make_port out$i OUTPUT 24 $y]
  make_inst NAND2_X1 g$i [list A1 $in A2 $a ZN $z] 4 $y
  set load [make_inst INV_X16 l$i [list A $z ZN $out] 20 $y]
  $load setDoNotTouch true
}
$in setDoNotTouch true
ord::design_created

# The A2 paths from a0..a11 and the side path through "in" fail.
create_clock -period 0.4 -name clk
set_input_delay -clock clk 0.1 [get_ports pi]
set_input_delay -clock clk 0.38 [get_ports a*]
set_output_delay -clock clk 0 [all_outputs]
set_output_delay -clock clk 0.24 [get_ports outs]
set_drive 0 [get_ports pi]
set_false_path -through [get_pins d1/ZN]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Masters of the siblings, and the slacks of both drivers of "in" and a
# sibling path.
proc report_state { } {
  set masters {}
  for { set i 0 } { $i < 12 } { incr i } {
    lappend masters [get_property [get_cells g$i] ref_name]
  }
  puts "g0..g11: $masters"
  puts [format "slack d0: %.4f d1: %s a0: %.4f" \
    [get_property [get_pins d0/ZN] slack_max] \
    [get_property [get_pins d1/ZN] slack_max] \
    [get_property [get_ports a0] slack_max]]
  report_worst_slack -max -digits 3
  report_tns -digits 3
}

report_state

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
