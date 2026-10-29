# One clock defined but registers on two clock nets: retime refuses the design.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_two_clocks.v
link_design top
create_clock -period 1.0 clk1

catch { retime } msg
puts $msg
