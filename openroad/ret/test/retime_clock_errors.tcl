# A design without registers has nothing to retime.  retime refuses a
# design with no clock or with two clocks, and runs once one of the two is
# deleted.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_clock_errors.v
link_design comb
create_clock -period 1.0 clk
retime
delete_clock clk

read_verilog retime_clock_errors.v
link_design top
catch { retime } msg
puts $msg

create_clock -period 1.0 clk1
create_clock -period 1.0 clk2
catch { retime } msg
puts $msg

delete_clock clk2
retime
