# A register whose data is a constant for OpenSTA stays.  A slide of rt and
# r1 across g1 would put the constant on g1, OpenSTA would stop timing the
# arcs the constant disables, and after an undo it would not time them again.
# g1 is fed through a register that stays, so nothing moves and the slack is
# the one from before.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_constant_data.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
