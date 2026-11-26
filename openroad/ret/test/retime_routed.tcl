# A design with routing is not retimed: retime warns and changes nothing.
# The wire is on q1, the net retime would delete with the register it moves.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set block [ord::get_db_block]
odb::dbWire_create [$block findNet q1]
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set net [$block findNet q1]
puts "q1 keeps its wire: [expr { $net ne "NULL" && [$net getWire] ne "NULL" }]"
