# A trial is judged on the slack of its netlist, next to a net with two
# drivers too.  The slide of ffa slows the path to po through da by more than
# it gains, so it is not kept; STA's incremental delay calculation alone would
# leave da at its old delay.  Timing everything again gives the same slack.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_shared_stale.v
link_design top
create_clock -period 0.23 clk
set_input_delay 0.0 -clock clk [get_ports {a b s}]
set_output_delay 0.07 -clock clk [get_ports po]

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
sta::delays_invalid
report_worst_slack -max -digits 4
