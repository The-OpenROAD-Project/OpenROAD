# The stop diagnosis of the slack search: where STA's failing endpoints are
# and what stops a slide at the start of each failing path of the model.
# Here the paths that still fail start at a primary input, at a register a
# port feeds, or at a spot whose slide would make the total worse.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_port_fed.v
link_design top
create_clock -period 0.05 clk

report_worst_slack -max -digits 4
set_debug_level RET slack_search 1
set_debug_level RET why 1
retime
