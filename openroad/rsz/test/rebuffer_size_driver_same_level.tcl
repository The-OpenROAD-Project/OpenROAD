# fully_rebuffer with set_opt_config -rebuffer_size_driver reads side-load
# slacks that do not depend on the order of drivers within one level.  Two
# copies of one circuit differ only in the name of sibling s, which sets
# whether s is rebuffered before or after x on their shared level.  In each,
# inverter d drives net n, which feeds x, s and g; s drives a far output.
# Rebuffering s first inserts a buffer that has no arrival until the level
# is propagated, so required times computed after it would leave s's input
# unconstrained and let x take INV_X4 at s's expense.  Both copies keep x at
# INV_X1.
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

# One copy at y0 um, with names suffixed by k and sibling s named s:
# in -> d -> n -> {x, s, g/A1, side}; b -> i1 -> i2 -> g/A2 puts g above x
# and s.  n and s are dont_touch, so only the output nets of x, s and g are
# rebuffered.
proc make_copy { k s y0 } {
  global block
  foreach name { in b n c1 c2 ns nx ng } {
    set $name [odb::dbNet_create $block $name$k]
  }
  make_port $in in$k INPUT 0 $y0
  make_port $b b$k INPUT 0 [expr { $y0 + 20 }]
  make_port $n side$k OUTPUT 12 $y0
  make_port $ns outs$k OUTPUT 200 $y0
  make_port $nx outx$k OUTPUT 20 [expr { $y0 + 10 }]
  make_port $ng outg$k OUTPUT 20 [expr { $y0 + 20 }]
  make_inst INV_X1 d$k [list A $in ZN $n] 2 $y0
  [make_inst INV_X1 $s [list A $n ZN $ns] 8 $y0] setDoNotTouch true
  make_inst INV_X1 x$k [list A $n ZN $nx] 8 [expr { $y0 + 10 }]
  make_inst INV_X4 i1$k [list A $b ZN $c1] 2 [expr { $y0 + 20 }]
  make_inst INV_X4 i2$k [list A $c1 ZN $c2] 4 [expr { $y0 + 20 }]
  make_inst NAND2_X1 g$k [list A1 $n A2 $c2 ZN $ng] 8 [expr { $y0 + 20 }]
  $n setDoNotTouch true
}

# Drivers on one level are visited in decreasing name order: z1 before x1,
# x2 before a2.
make_copy 1 z1 0
make_copy 2 a2 100
ord::design_created

create_clock -period 0.5 -name clk
foreach k { 1 2 } {
  set_input_delay -clock clk 0 [get_ports in$k]
  set_input_delay -clock clk 0.3 [get_ports b$k]
  set_output_delay -clock clk 0.3 [get_ports outg$k]
  set_output_delay -clock clk 0.38 [get_ports outx$k]
  set_output_delay -clock clk 0.38 [get_ports outs$k]
  set_load 40 [get_ports outg$k]
  set_load 60 [get_ports outx$k]
  set_load 28.5 [get_ports side$k]
  set_load 30 [get_ports outs$k]
}
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Masters of x and g and the slacks of s and x in each copy.
proc report_state { } {
  foreach { k s } { 1 z1 2 a2 } {
    puts [format "copy %s: x %s g %s slack %s/A: %.3f x/ZN: %.3f" \
      $k [get_property [get_cells x$k] ref_name] \
      [get_property [get_cells g$k] ref_name] $s \
      [get_property [get_pins $s/A] slack_max] \
      [get_property [get_pins x$k/ZN] slack_max]]
  }
  report_worst_slack -max -digits 3
  report_tns -digits 3
}

report_state

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_state

reset_opt_config -rebuffer_size_driver
