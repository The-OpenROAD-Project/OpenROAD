# A register that reads a net with two drivers stays, on a failing path too:
# a move would put a new load on that net, and OpenSTA's delay calculation of
# a net with two drivers does not take one.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_bus_input.v
link_design top
create_clock -period 0.1 clk

retime
set g1_input [[[ord::get_db_block] findInst g1] findITerm A]
puts "g1/A reads [[$g1_input getNet] getName]"
