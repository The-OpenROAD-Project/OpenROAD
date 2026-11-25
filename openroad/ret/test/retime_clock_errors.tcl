# retime refuses a design with no clock or with two clocks.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_clock_errors.v
link_design top

# No clock.
catch { retime } msg
puts $msg

# Two clocks.
create_clock -period 1.0 clk1
create_clock -period 1.0 clk2
catch { retime } msg
puts $msg
