# fully_rebuffer with set_opt_config -rebuffer_size_driver plans a buffer tree
# for the fastest driver cell that keeps the upstream side loads feasible.
# Inverter x drives a failing net.  INV_X8 is the fastest x, but its input
# capacitance slows the near-critical side output on x's input net too much,
# so the tree is planned for INV_X4, and x takes INV_X4 with that tree
# instead of the tree buffered for INV_X2.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]

# Top-level port on net at (x, y) um.
proc make_port { net name io x y } {
  global block pin_layer
  set bterm [odb::dbBTerm_create $net $name]
  $bterm setIoType $io
  set bpin [odb::dbBPin_create $bterm]
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]
  odb::dbBox_create $bpin $pin_layer $x $y [expr { $x + 100 }] [expr { $y + 100 }]
  $bpin setPlacementStatus LOCKED
}

# Inverter at (x, y) um from in_net to out_net.
proc make_inv { master name in_net out_net x y } {
  global block db
  set inst [odb::dbInst_create $block [$db findMaster $master] $name]
  [$inst findITerm A] connect $in_net
  [$inst findITerm ZN] connect $out_net
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus PLACED
}

# in -> d -> p -> {x, side}; x drives six outputs 100 um away.  p is
# dont_touch, so only x's net is rebuffered.
set in [odb::dbNet_create $block in]
set p [odb::dbNet_create $block p]
set n [odb::dbNet_create $block n]
make_port $in in INPUT 0 0
make_inv INV_X4 d $in $p 5 0
make_inv INV_X2 x $p $n 10 0
make_port $p side OUTPUT 12 0
for { set i 0 } { $i < 6 } { incr i } {
  make_port $n out$i OUTPUT [expr { 100 + 5 * ($i % 3) }] [expr { 5 * ($i / 3) }]
}
$p setDoNotTouch true
ord::design_created

# x's net is the worst; side has only a few ps more slack than x's
# rebuffered net.  The side output delay is tuned so INV_X8 fails the
# side-load check and INV_X4 passes it.
create_clock -period 0.05 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [get_ports out*]
set_output_delay -clock clk 0.0522 [get_ports side]
set_load 10 [get_ports out*]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# x's master, the buffers on its net, and the slacks of x and side.
proc report_x_net { } {
  set block [ord::get_db_block]
  set buffers {}
  foreach inst [$block getInsts] {
    if { [$inst getName] ni {d x} } {
      lappend buffers [[$inst getMaster] getName]
    }
  }
  puts "x: [[[$block findInst x] getMaster] getName] buffers: [lsort $buffers]"
  puts [format "slack x/ZN: %.3f side: %.3f" \
    [get_property [get_pins x/ZN] slack_max] \
    [get_property [get_ports side] slack_max]]
  report_tns -digits 3
}

report_x_net

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_x_net

reset_opt_config -rebuffer_size_driver
