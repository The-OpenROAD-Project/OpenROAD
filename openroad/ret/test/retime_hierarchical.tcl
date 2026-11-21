# A hierarchical netlist is an error, not a silent no-op: new cells would
# have to be created inside the right module.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_hierarchical.v
link_design -hier top
create_clock -period 1.0 clk

catch { retime } msg
puts $msg
