# In a hierarchical netlist a register stays when a cell in another module
# reads it: its loads are rewired with it, and that one would end on a net
# of the wrong module.  rx in module spy reads r1 of module core and drives
# nothing, so no path leaves it; r1 and r2 still do not cross g2, and rx
# reads the net it read before.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_hier_reader.v
link_design -hier top
create_clock -period 0.15 clk

retime
check_axioms
set pin [[ord::get_db_block] findITerm u_spy/rx/D]
puts "u_spy/rx/D reads [[$pin getNet] getName], in spy [[$pin getModNet] getName]"
