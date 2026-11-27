# A design with routing is not retimed: retime warns and changes nothing.
# The wire is on q1, the net retime would delete with the register it moves.
# A special wire on q1 is routing too.  The special wires of the supply nets,
# which every design has after pdngen, are not: with only those left, retime
# moves the register.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set block [ord::get_db_block]
foreach {name type} {VDD POWER VSS GROUND} {
  set net [odb::dbNet_create $block $name]
  $net setSigType $type
  $net setSpecial
  odb::dbSWire_create $net ROUTED
}
set q1 [$block findNet q1]
odb::dbWire_create $q1
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set net [$block findNet q1]
puts "q1 keeps its wire: [expr { $net ne "NULL" && [$net getWire] ne "NULL" }]"

odb::dbWire_destroy [$q1 getWire]
odb::dbSWire_create $q1 ROUTED
retime
set net [$block findNet q1]
puts "q1 keeps its special wire: [expr { $net ne "NULL" && [llength [$net getSWires]] == 1 }]"

odb::dbSWire_destroy [lindex [$q1 getSWires] 0]
retime
foreach name {VDD VSS} {
  puts "$name special wires: [llength [[$block findNet $name] getSWires]]"
}
