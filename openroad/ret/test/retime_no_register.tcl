# A design without registers has nothing to retime.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_no_register.v
link_design top
create_clock -period 0.1 clk

retime
