# fully_rebuffer with set_opt_config -rebuffer_size_driver does not pair a
# collapsed area tree with a resized logic driver.  Inverter pbuf drives one
# BUF_X8, which drives 36 loads 100 um away.  The net passes, and area
# recovery removes the BUF_X8, leaving pbuf with 36 direct sinks, as it does
# without driver sizing.  That flat tree stays with INV_X1; before, sizing
# upsized pbuf to INV_X2 to drive the 36 sinks.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip top]
$block setDefUnits [$tech getDbUnitsPerMicron]
set pin_layer [$tech findLayer metal1]
set buf_x1 [$db findMaster BUF_X1]
set buf_x8 [$db findMaster BUF_X8]

# Net driven from a placed top-level port.
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

# Placed buffer from in_net to out_net.
proc make_buffer { master name in_net out_net x y } {
  global block
  set inst [odb::dbInst_create $block $master $name]
  [$inst findITerm A] connect $in_net
  [$inst findITerm Z] connect $out_net
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  return $inst
}

# Inverter pbuf drives one BUF_X8, which drives 36 loads 100 um away.
set in_net [make_port in INPUT 0 0]
set n0 [odb::dbNet_create $block n0]
set n1 [odb::dbNet_create $block n1]
set pbuf [odb::dbInst_create $block [$db findMaster INV_X1] pbuf]
[$pbuf findITerm A] connect $in_net
[$pbuf findITerm ZN] connect $n0
$pbuf setLocation [ord::microns_to_dbu 10] 0
$pbuf setPlacementStatus PLACED
make_buffer $buf_x8 b8 $n0 $n1 [ord::microns_to_dbu 60] 0
for { set i 0 } { $i < 36 } { incr i } {
  set x [ord::microns_to_dbu [expr { 110 + 5 * ($i % 6) }]]
  set y [ord::microns_to_dbu [expr { 5 * ($i / 6) }]]
  set out_net [make_port out$i OUTPUT [expr { $x + [ord::microns_to_dbu 2] }] $y]
  make_buffer $buf_x1 load$i $n1 $out_net $x $y
}
ord::design_created

create_clock -period 10 -name clk
set_input_delay -clock clk 0 [all_inputs]
set_output_delay -clock clk 0 [all_outputs]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# The master of pbuf, its direct sinks, and every other buffer that is
# not a load.
proc report_port_net { } {
  set block [ord::get_db_block]
  set pbuf [$block findInst pbuf]
  set net [[$pbuf findITerm ZN] getNet]
  set sinks [expr { [llength [$net getITerms]] - 1 }]
  set buffers {}
  foreach inst [$block getInsts] {
    set name [$inst getName]
    if { $name ne "pbuf" && ![string match load* $name] } {
      lappend buffers [[$inst getMaster] getName]
    }
  }
  puts "pbuf: [[$pbuf getMaster] getName] direct sinks: $sinks\
    buffers: [lsort $buffers]"
}

report_port_net
report_worst_slack -max

set_opt_config -rebuffer_size_driver true
rsz::fully_rebuffer NULL

estimate_parasitics -placement
report_port_net
report_worst_slack -max

reset_opt_config -rebuffer_size_driver
