# A register whose input net has two drivers moves like any other.  Both
# drivers are read again after the move, so the search of the next round
# runs on the netlist as it is then: g1 reads the bus, and round 2 finds
# nothing more to move.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_bus_input.v
link_design top
create_clock -period 0.1 clk

retime
set g1_input [[[ord::get_db_block] findInst g1] findITerm A]
puts "g1/A reads [[$g1_input getNet] getName]"
