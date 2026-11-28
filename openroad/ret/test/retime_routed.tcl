# A design with routing is not retimed: retime warns and changes nothing.
# The wire is on q1, the net retime would delete with the register it
# moves.  A special wire on q1 is routing too.  The special wires of the
# supply nets, which every design has after pdngen, are not: with only
# those left, retime moves the register.  Read again, the same design is
# also left alone, with a warning, with delays annotated from SDF and then
# with parasitics read from SPEF: STA drops an annotated delay with its
# wire and takes a disconnected pin off its SPEF network, and a trial that
# is undone brings neither back.  The hold uncertainty makes STA reject
# every trial there, so a retime that ran would undo each one.  SPEF goes
# last: reading the design again does not clear it.
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
set kept [expr { $net ne "NULL" && [$net getWire] ne "NULL" }]
puts "q1 keeps its wire: $kept"

odb::dbWire_destroy [$q1 getWire]
odb::dbSWire_create $q1 ROUTED
retime
set net [$block findNet q1]
set kept [expr { $net ne "NULL" && [llength [$net getSWires]] == 1 }]
puts "q1 keeps its special wire: $kept"

odb::dbSWire_destroy [lindex [$q1 getSWires] 0]
retime
foreach name {VDD VSS} {
  puts "$name special wires: [llength [[$block findNet $name] getSWires]]"
}

read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk
set_clock_uncertainty -hold 0.30 clk
read_sdf retime_routed.sdf
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk
set_clock_uncertainty -hold 0.30 clk
read_spef retime_routed.spef
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
